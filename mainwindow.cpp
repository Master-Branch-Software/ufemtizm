#include "mainwindow.hpp"
#include "timezonewidget.hpp"
#include "settingsdialog.hpp"
#include "version.h"
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QCloseEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QIcon>
#include <QToolBar>
#include <QSystemTrayIcon>
#include <QEvent>
#include <QTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QApplication>
#include <QWindow>
#include <QClipboard>
#include <QCursor>
#include <QPalette>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent),
    isDirty(false),
    trayIcon(nullptr),
    trayMenu(nullptr),
    trayRecentFilesMenu(nullptr),
    settingsDialog(nullptr),
    localServer(nullptr),
    forceQuit(false),
    initialSizeSet(false),
    trayInitTimer(nullptr),
    trayInitAttempts(0){
    setStyleSheet(
        "QMainWindow {"
        "    background-color: #f6fafe;"
        "}"
        "QMenuBar {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    padding: 4px;"
        "}"
        "QMenuBar::item {"
        "    background-color: transparent;"
        "    padding: 6px 12px;"
        "    border-radius: 8px;"
        "    color: #2a343a;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #eef4fa;"
        "}"
        "QMenu {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 12px;"
        "    padding: 6px;"
        "}"
        "QMenu::item {"
        "    padding: 8px 24px 8px 12px;"
        "    border-radius: 8px;"
        "    color: #2a343a;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #eef4fa;"
        "}"
        "QMenu::separator {"
        "    height: 1px;"
        "    background-color: #e7eff5;"
        "    margin: 4px 8px;"
        "}"
    );
    
    QIcon icon = QIcon::fromTheme("unfuck-my-timezone-math");
    if (icon.isNull()){
        icon = QIcon("/usr/share/icons/hicolor/256x256/apps/unfuck-my-timezone-math.png");
    }

    if (!icon.isNull()){
        setWindowIcon(icon);
    }
    
    setupMenuBar();
    setupToolBar();
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    
    localServer = new QLocalServer(this);
    QString serverName = "UnfuckMyTimeZoneMath_SingleInstance";
    QLocalServer::removeServer(serverName);

    if (localServer->listen(serverName)){
        connect(localServer, &QLocalServer::newConnection, this, &MainWindow::handleNewConnection);
    }
    
    initializeSystemTray();
    
    setAttribute(Qt::WA_QuitOnClose, false);
    
    // Fix black window on Linux by setting palette background before first paint
    QPalette windowPalette = palette();
    windowPalette.setColor(QPalette::Window, QColor(0xf6, 0xfa, 0xfe));
    setPalette(windowPalette);
    setAutoFillBackground(true);

#ifdef Q_OS_LINUX
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, false);
#endif
    
    // Hide from taskbar on Windows only (macOS uses LSUIElement in Info.plist)
    // On Linux, we want the window to appear in Alt+Tab when visible
#ifdef Q_OS_WIN
    setWindowFlags(windowFlags() | Qt::Tool);
#endif
    
    centralWidget = new QWidget();
    centralWidget->setAutoFillBackground(true);

    QPalette centralPalette = centralWidget->palette();
    centralPalette.setColor(QPalette::Window, QColor(0xf6, 0xfa, 0xfe));
    centralWidget->setPalette(centralPalette);

    QHBoxLayout *layout = new QHBoxLayout(centralWidget);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    setCentralWidget(centralWidget);
    
    addTimeZoneWidget();
    updateWindowTitle();
    isDirty = false;
    
    QStringList recentFiles = getRecentFiles();

    if (!recentFiles.isEmpty()){
        QString lastFile = recentFiles.first();

        if (QFile::exists(lastFile)){
            loadFromFile(lastFile);
        }
        else{
            QMessageBox::warning(this, "File Not Found",
                                QString("The last opened file was not found:\n%1\n\nStarting with a new file.").arg(lastFile));
        }
    }
    
}

MainWindow::~MainWindow()
{
}

void MainWindow::mousePressEvent(QMouseEvent *event){
    for (TimeZoneWidget *widget : timeZoneWidgets){
        QPoint widgetPos = widget->mapFrom(this, event->pos());
        if (widget->rect().contains(widgetPos)){
            QMainWindow::mousePressEvent(event);
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::setupMenuBar(){
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    QAction *newAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/document-new.svg"), "&New");
    newAction->setShortcut(QKeySequence::New);
    newAction->setToolTip("Create a new timezone configuration");
    newAction->setStatusTip("Create a new timezone configuration");
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    
    QAction *openAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/document-open.svg"), "&Open...");
    openAction->setShortcut(QKeySequence::Open);
    openAction->setToolTip("Open an existing timezone configuration");
    openAction->setStatusTip("Open an existing timezone configuration");
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    
    recentFilesMenu = fileMenu->addMenu(QIcon(":/toolbar/toolbar-icons/document-open.svg"), "Recent Files");
    updateRecentFilesMenu();
    
    fileMenu->addSeparator();
    
    QAction *saveAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/document-save.svg"), "&Save");
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setToolTip("Save the current configuration");
    saveAction->setStatusTip("Save the current configuration");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    
    QAction *saveAsAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/document-save-as.svg"), "Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setToolTip("Save the configuration with a new name");
    saveAsAction->setStatusTip("Save the configuration with a new name");
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    
    fileMenu->addSeparator();
    
    QAction *addAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/list-add.svg"), "Add Time &Zone");
    addAction->setShortcut(QKeySequence("Ctrl+T"));
    addAction->setToolTip("Add a new timezone widget (Ctrl+T)");
    addAction->setStatusTip("Add a new timezone widget");
    connect(addAction, &QAction::triggered, this, &MainWindow::addTimeZoneWidget);
    
    fileMenu->addSeparator();
    
    QAction *currentTimeAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/clock.svg"), "Set to Current Time");
    currentTimeAction->setShortcut(QKeySequence("Alt+T"));
    currentTimeAction->setToolTip("Set all timezones to current time (Alt+T)");
    currentTimeAction->setStatusTip("Set all timezones to current time");
    connect(currentTimeAction, &QAction::triggered, this, &MainWindow::setToCurrentTime);
    
    fileMenu->addSeparator();
    
    QAction *settingsAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/configure.svg"), "&Settings...");
    settingsAction->setToolTip("Configure application settings");
    settingsAction->setStatusTip("Configure application settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/application-exit.svg"), "&Quit");
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setToolTip("Exit the application");
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    
    QAction *copyAction = editMenu->addAction(QIcon(":/toolbar/toolbar-icons/edit-copy.svg"), "&Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setToolTip("Copy timezone information to clipboard (Ctrl+C)");
    copyAction->setStatusTip("Copy timezone information to clipboard");
    connect(copyAction, &QAction::triggered, this, &MainWindow::copyToClipboard);
    
    QMenu *viewMenu = menuBar()->addMenu("&View");
    
    toggleToolBarAction = viewMenu->addAction("Show &Toolbar");
    toggleToolBarAction->setCheckable(true);
    toggleToolBarAction->setChecked(true);
    toggleToolBarAction->setToolTip("Toggle toolbar visibility");
    toggleToolBarAction->setStatusTip("Toggle toolbar visibility");
    connect(toggleToolBarAction, &QAction::triggered, this, &MainWindow::toggleToolBarVisibility);
    
    toggleToolBarTextAction = viewMenu->addAction("Show Toolbar &Text");
    toggleToolBarTextAction->setCheckable(true);
    toggleToolBarTextAction->setChecked(true);
    toggleToolBarTextAction->setToolTip("Toggle toolbar text labels");
    toggleToolBarTextAction->setStatusTip("Toggle toolbar text labels");
    connect(toggleToolBarTextAction, &QAction::triggered, this, &MainWindow::toggleToolBarTextVisibility);
    
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    
    QAction *aboutAction = helpMenu->addAction("&About");
    aboutAction->setToolTip("About this application");
    aboutAction->setStatusTip("About this application");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::setupToolBar(){
    toolBar = addToolBar("Main Toolbar");
    toolBar->setObjectName("MainToolBar");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(22, 22));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setStyleSheet(
        "QToolBar {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    spacing: 4px;"
        "    padding: 4px;"
        "}"
        "QToolButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px;"
        "    margin: 2px;"
        "    color: #2a343a;"
        "}"
        "QToolButton:hover {"
        "    background-color: #eef4fa;"
        "}"
        "QToolButton:pressed {"
        "    background-color: #e7eff5;"
        "}"
    );
    
    QAction *newAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/document-new.svg"), "New");
    newAction->setToolTip("Create a new timezone configuration (Ctrl+N)");
    newAction->setStatusTip("Create a new timezone configuration");
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    mainToolBarActions.append(newAction);
    
    QAction *openAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/document-open.svg"), "Open");
    openAction->setToolTip("Open an existing timezone configuration (Ctrl+O)");
    openAction->setStatusTip("Open an existing timezone configuration");
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    mainToolBarActions.append(openAction);
    
    QAction *saveAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/document-save.svg"), "Save");
    saveAction->setToolTip("Save the current configuration (Ctrl+S)");
    saveAction->setStatusTip("Save the current configuration");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    mainToolBarActions.append(saveAction);
    
    toolBar->addSeparator();
    
    QAction *copyAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/edit-copy.svg"), "Copy");
    copyAction->setToolTip("Copy timezone information to clipboard (Ctrl+C)");
    copyAction->setStatusTip("Copy timezone information to clipboard");
    connect(copyAction, &QAction::triggered, this, &MainWindow::copyToClipboard);
    mainToolBarActions.append(copyAction);
    
    QAction *addAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/list-add.svg"), "Add Zone");
    addAction->setToolTip("Add a new timezone widget (Ctrl+T)");
    addAction->setStatusTip("Add a new timezone widget");
    connect(addAction, &QAction::triggered, this, &MainWindow::addTimeZoneWidget);
    mainToolBarActions.append(addAction);
    
    QAction *currentTimeAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/clock.svg"), "Current Time");
    currentTimeAction->setToolTip("Set all timezones to current time (Alt+T)");
    currentTimeAction->setStatusTip("Set all timezones to current time");
    connect(currentTimeAction, &QAction::triggered, this, &MainWindow::setToCurrentTime);
    mainToolBarActions.append(currentTimeAction);
    
    toolBar->addSeparator();
    
    QAction *skyColorAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/colors.svg"), "Colors");
    skyColorAction->setCheckable(true);
    QSettings skySettings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    skyColorAction->setChecked(skySettings.value("appearance/skyColor", true).toBool());
    skyColorAction->setToolTip("Toggle sky color theme");
    skyColorAction->setStatusTip("Toggle sky color theme based on time of day");
    connect(skyColorAction, &QAction::triggered, this, &MainWindow::toggleSkyColor);
    mainToolBarActions.append(skyColorAction);
    
    QAction *settingsToolBarAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/configure.svg"), "Settings");
    settingsToolBarAction->setToolTip("Configure application settings");
    settingsToolBarAction->setStatusTip("Configure application settings");
    connect(settingsToolBarAction, &QAction::triggered, this, &MainWindow::showSettings);
    mainToolBarActions.append(settingsToolBarAction);
    
    toolBar->addSeparator();
    
    toolBarTextToggleAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/draw-text.svg"), "");
    toolBarTextToggleAction->setCheckable(true);
    toolBarTextToggleAction->setChecked(true);
    toolBarTextToggleAction->setToolTip("Toggle toolbar text labels");
    toolBarTextToggleAction->setStatusTip("Toggle toolbar text labels");
    connect(toolBarTextToggleAction, &QAction::triggered, this, &MainWindow::toggleToolBarTextVisibility);
    
    toolBarHideAction = toolBar->addAction(QIcon(":/toolbar/toolbar-icons/window-close.svg"), "");
    toolBarHideAction->setToolTip("Hide toolbar");
    toolBarHideAction->setStatusTip("Hide toolbar");
    connect(toolBarHideAction, &QAction::triggered, this, &MainWindow::toggleToolBarVisibility);
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    bool toolBarVisible = settings.value("toolBarVisible", true).toBool();
    toolBar->setVisible(toolBarVisible);
    
    bool toolBarTextVisible = settings.value("toolBarTextVisible", true).toBool();

    if (!toolBarTextVisible){
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    else{
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    
    if (toggleToolBarAction){
        toggleToolBarAction->setChecked(toolBarVisible);
    }

    if (toggleToolBarTextAction){
        toggleToolBarTextAction->setChecked(toolBarTextVisible);
    }

    if (toolBarTextToggleAction){
        toolBarTextToggleAction->setChecked(toolBarTextVisible);
    }
}

void MainWindow::addTimeZoneWidget(){
    TimeZoneWidget *widget = new TimeZoneWidget(centralWidget);
    
    connect(widget, &TimeZoneWidget::timeChanged, this, &MainWindow::onTimeChanged);
    connect(widget, &TimeZoneWidget::removeRequested, this, &MainWindow::removeTimeZoneWidget);
    connect(widget, &TimeZoneWidget::widgetModified, this, &MainWindow::onWidgetModified);
    connect(widget, &TimeZoneWidget::dropReceived, this, &MainWindow::onWidgetDropped);

    if (!timeZoneWidgets.isEmpty()){
        widget->setBaseTimestamp(timeZoneWidgets.first()->getBaseTimestamp());
        widget->setIs24HourFormat(timeZoneWidgets.first()->getIs24HourFormat());
    }
    
    timeZoneWidgets.append(widget);
    centralWidget->layout()->addWidget(widget);
    
    widget->selectName();
    
    adjustWindowSize();
    
    isDirty = true;
    updateWindowTitle();
}

void MainWindow::removeTimeZoneWidget(TimeZoneWidget *widget){
    if (timeZoneWidgets.size() <= 1){
        return;
    }
    
    timeZoneWidgets.removeOne(widget);
    widget->deleteLater();
    
    adjustWindowSize();
    
    isDirty = true;
    updateWindowTitle();
}

void MainWindow::onTimeChanged(qint64 baseTimestamp){
    TimeZoneWidget *source = qobject_cast<TimeZoneWidget*>(sender());
    updateAllWidgets(baseTimestamp, source);
}

void MainWindow::updateAllWidgets(qint64 baseTimestamp, TimeZoneWidget *source){
    for (TimeZoneWidget *widget : timeZoneWidgets){
        if (widget != source){
            widget->setBaseTimestamp(baseTimestamp);
        }
    }
}

void MainWindow::onWidgetModified(){
    isDirty = true;
    updateWindowTitle();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    bool closeToTray = settings.value("systemTray/closeToTray", true).toBool();
    
    if (!forceQuit && closeToTray && trayIcon && trayIcon->isVisible()){
        hide();
        event->ignore();
        return;
    }
    
    if (maybeSave()){
        saveWindowGeometry();
        event->accept();
    }
    else{
        event->ignore();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange){
        QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
        bool minimizeToTray = settings.value("systemTray/minimizeToTray", true).toBool();
        
        if (isMinimized() && minimizeToTray && trayIcon && trayIcon->isVisible()){
            QTimer::singleShot(0, this, &QWidget::hide);
            event->ignore();
            return;
        }
    }
    
    QMainWindow::changeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    
    if (!initialSizeSet){
        initialSizeSet = true;
        adjustWindowSize();
        restoreWindowGeometry();
    }

#ifdef Q_OS_LINUX
    QTimer::singleShot(0, this, [this](){
        update();
        if (centralWidget){
            centralWidget->update();
        }
    });
#endif
}

void MainWindow::newFile(){
    if (!maybeSave()){
        return;
    }
    
    while (timeZoneWidgets.size() > 1){
        TimeZoneWidget *widget = timeZoneWidgets.last();
        timeZoneWidgets.removeLast();
        widget->deleteLater();
    }
    
    if (!timeZoneWidgets.isEmpty()){
        timeZoneWidgets.first()->setFriendlyName("My Wonderful Self");
    }
    
    adjustWindowSize();
    
    currentFilename.clear();
    isDirty = false;
    updateWindowTitle();
}

void MainWindow::openFile(){
    if (!maybeSave()){
        return;
    }
    
    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Configuration");
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("YAML Files (*.yaml *.yml)");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setStyleSheet(
        "QFileDialog {"
        "    background-color: #f6fafe;"
        "}"
        "QDialog {"
        "    background-color: #f6fafe;"
        "}"
        "QPushButton {"
        "    background-color: #eef4fa;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 16px;"
        "    min-width: 80px;"
        "    color: #2a343a;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e7eff5;"
        "}"
        "QPushButton:default {"
        "    background-color: #4e45e4;"
        "    color: #fbf7ff;"
        "    border: none;"
        "}"
        "QPushButton:default:hover {"
        "    background-color: #4135d8;"
        "}"
        "QListView, QTreeView {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 4px;"
        "}"
        "QListView::item:hover, QTreeView::item:hover {"
        "    background-color: #eef4fa;"
        "}"
        "QListView::item:selected, QTreeView::item:selected {"
        "    background-color: #e7eff5;"
        "    color: #2a343a;"
        "}"
        "QLineEdit {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid #4e45e4;"
        "}"
        "QComboBox {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px;"
        "}"
    );
    
    if (dialog.exec() == QDialog::Accepted){
        QStringList files = dialog.selectedFiles();
        if (!files.isEmpty()){
            loadFromFile(files.first());
        }
    }
}

void MainWindow::saveFile(){
    if (currentFilename.isEmpty()){
        saveFileAs();
    }
    else{
        saveToFile(currentFilename);
    }
}

void MainWindow::saveFileAs(){
    QFileDialog dialog(this);
    dialog.setWindowTitle("Save Configuration");
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("YAML Files (*.yaml *.yml)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("yaml");
    dialog.setStyleSheet(
        "QFileDialog {"
        "    background-color: #f6fafe;"
        "}"
        "QDialog {"
        "    background-color: #f6fafe;"
        "}"
        "QPushButton {"
        "    background-color: #eef4fa;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 16px;"
        "    min-width: 80px;"
        "    color: #2a343a;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e7eff5;"
        "}"
        "QPushButton:default {"
        "    background-color: #4e45e4;"
        "    color: #fbf7ff;"
        "    border: none;"
        "}"
        "QPushButton:default:hover {"
        "    background-color: #4135d8;"
        "}"
        "QListView, QTreeView {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 4px;"
        "}"
        "QListView::item:hover, QTreeView::item:hover {"
        "    background-color: #eef4fa;"
        "}"
        "QListView::item:selected, QTreeView::item:selected {"
        "    background-color: #e7eff5;"
        "    color: #2a343a;"
        "}"
        "QLineEdit {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid #4e45e4;"
        "}"
        "QComboBox {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px;"
        "}"
    );
    
    if (dialog.exec() == QDialog::Accepted){
        QStringList files = dialog.selectedFiles();
        if (!files.isEmpty()){
            QString filename = files.first();
            if (!filename.endsWith(".yaml", Qt::CaseInsensitive) && !filename.endsWith(".yml", Qt::CaseInsensitive)){
                filename += ".yaml";
            }
            saveToFile(filename);
        }
    }
}

void MainWindow::openRecentFile(){
    QAction *action = qobject_cast<QAction*>(sender());
    
    if (action){
        if (!maybeSave()){
            return;
        }
        
        loadFromFile(action->data().toString());
    }
}

void MainWindow::updateWindowTitle(){
    QString title;
    
    if (currentFilename.isEmpty()){
        title = "Untitled";
    }
    else{
        QFileInfo fileInfo(currentFilename);
        title = fileInfo.completeBaseName();
    }
    
    if (isDirty){
        title += " *";
    }
    
    title += " - UnfuckMyTimeZoneMath";
    setWindowTitle(title);
}

void MainWindow::updateRecentFilesMenu(){
    recentFilesMenu->clear();
    
    QStringList recentFiles = getRecentFiles();
    
    if (recentFiles.isEmpty()){
        QAction *noFilesAction = recentFilesMenu->addAction("No Recent Files");
        noFilesAction->setEnabled(false);
    }
    else{
        for (const QString &filename : recentFiles){
            QFileInfo fileInfo(filename);
            QString displayName = fileInfo.completeBaseName();
            QAction *action = recentFilesMenu->addAction(displayName);
            action->setData(filename);
            connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
        }
    }
    
    updateTrayMenu();
}

bool MainWindow::maybeSave(){
    if (!isDirty){
        return true;
    }
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText("You have unsaved changes.");
    msgBox.setInformativeText("Do you want to save your changes before continuing?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    msgBox.button(QMessageBox::Discard)->setText("Discard");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #f6fafe;"
        "}"
        "QPushButton {"
        "    background-color: #eef4fa;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px 16px;"
        "    min-width: 70px;"
        "    color: #2a343a;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e7eff5;"
        "}"
        "QPushButton:default {"
        "    background-color: #4e45e4;"
        "    color: #fbf7ff;"
        "    border: none;"
        "}"
        "QPushButton:default:hover {"
        "    background-color: #4135d8;"
        "}"
    );
    
    int reply = msgBox.exec();
    
    if (reply == QMessageBox::Save){
        saveFile();
        return !isDirty;
    }
    else if (reply == QMessageBox::Discard)
    {
        return true;
    }
    else{
        return false;
    }
}

bool MainWindow::saveToFile(const QString &filename)
{
    QFile file(filename);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::critical(this, "Error", "Could not save file: " + file.errorString());
        return false;
    }
    
    QTextStream out(&file);
    out << "timezones:\n";
    
    for (TimeZoneWidget *widget : timeZoneWidgets){
        out << "  - name: \"" << widget->getFriendlyName() << "\"\n";
        out << "    timezone: \"" << widget->getTimeZoneId() << "\"\n";
        out << "    format24hour: " << (widget->getIs24HourFormat() ? "true" : "false") << "\n";
    }
    
    file.close();
    
    setCurrentFile(filename);
    isDirty = false;
    updateWindowTitle();
    
    return true;
}

bool MainWindow::loadFromFile(const QString &filename)
{
    QFile file(filename);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::critical(this, "Error", "Could not open file: " + file.errorString());
        return false;
    }
    
    while (!timeZoneWidgets.isEmpty()){
        TimeZoneWidget *widget = timeZoneWidgets.takeLast();
        widget->deleteLater();
    }
    
    QTextStream in(&file);
    QString line;
    QString currentName;
    QString currentTz;
    bool current24Hour = true;
    bool inTimezones = false;
    bool inEntry = false;
    
    while (!in.atEnd()){
        line = in.readLine().trimmed();
        
        if (line == "timezones:"){
            inTimezones = true;
            continue;
        }
        
        if (inTimezones && line.startsWith("- name:")){
            if (inEntry && !currentName.isEmpty()){
                TimeZoneWidget *widget = new TimeZoneWidget(centralWidget);
                widget->setFriendlyName(currentName);
                widget->setTimeZoneId(currentTz);
                widget->setIs24HourFormat(current24Hour);
                
                connect(widget, &TimeZoneWidget::timeChanged, this, &MainWindow::onTimeChanged);
                connect(widget, &TimeZoneWidget::removeRequested, this, &MainWindow::removeTimeZoneWidget);
                connect(widget, &TimeZoneWidget::widgetModified, this, &MainWindow::onWidgetModified);
                connect(widget, &TimeZoneWidget::dropReceived, this, &MainWindow::onWidgetDropped);
                
                timeZoneWidgets.append(widget);
                centralWidget->layout()->addWidget(widget);
            }
            
            inEntry = true;
            currentName = line.mid(7).trimmed();
            currentName.remove('"');
            currentTz.clear();
            current24Hour = true;
        }
        else if (line.startsWith("timezone:"))
        {
            currentTz = line.mid(9).trimmed();
            currentTz.remove('"');
        }
        else if (line.startsWith("format24hour:"))
        {
            QString value = line.mid(13).trimmed();
            current24Hour = (value == "true");
        }
    }
    
    if (inEntry && !currentName.isEmpty()){
        TimeZoneWidget *widget = new TimeZoneWidget(centralWidget);
        widget->setFriendlyName(currentName);
        widget->setTimeZoneId(currentTz);
        widget->setIs24HourFormat(current24Hour);
        
        connect(widget, &TimeZoneWidget::timeChanged, this, &MainWindow::onTimeChanged);
        connect(widget, &TimeZoneWidget::removeRequested, this, &MainWindow::removeTimeZoneWidget);
        connect(widget, &TimeZoneWidget::widgetModified, this, &MainWindow::onWidgetModified);
        connect(widget, &TimeZoneWidget::dropReceived, this, &MainWindow::onWidgetDropped);
        
        timeZoneWidgets.append(widget);
        centralWidget->layout()->addWidget(widget);
    }
    
    if (timeZoneWidgets.isEmpty()){
        addTimeZoneWidget();
    }
    else{
        qint64 currentTime = QDateTime::currentSecsSinceEpoch();
        
        for (TimeZoneWidget *widget : timeZoneWidgets){
            widget->setBaseTimestamp(currentTime);
        }
        
        adjustWindowSize();
    }
    
    file.close();
    
    setCurrentFile(filename);
    isDirty = false;
    updateWindowTitle();
    
    return true;
}

void MainWindow::setCurrentFile(const QString &filename)
{
    currentFilename = filename;
    addRecentFile(filename);
    updateRecentFilesMenu();
}

void MainWindow::addRecentFile(const QString &filename)
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    
    recentFiles.removeAll(filename);
    recentFiles.prepend(filename);
    
    while (recentFiles.size() > 10){
        recentFiles.removeLast();
    }
    
    settings.setValue("recentFiles", recentFiles);
}

QStringList MainWindow::getRecentFiles() const
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    
    QStringList existingFiles;
    
    for (const QString &filename : recentFiles){
        if (QFile::exists(filename)){
            existingFiles.append(filename);
        }
    }
    
    return existingFiles;
}

void MainWindow::adjustWindowSize(){
    int widgetWidth = 240;
    int widgetCount = timeZoneWidgets.size();
    int spacing = 12;
    int margins = 32;
    
    int totalWidth = (widgetWidth * widgetCount) + (spacing * (widgetCount - 1)) + margins;
    int contentHeight = 700;
    
    int menuBarHeight = menuBar()->height();
    int toolBarHeight = toolBar->isVisible() ? toolBar->height() : 0;
    
    int totalHeight = contentHeight + menuBarHeight + toolBarHeight;
    
    setFixedSize(totalWidth, totalHeight);
}

void MainWindow::saveWindowGeometry(){
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    settings.setValue("windowPosition", pos());
}

void MainWindow::restoreWindowGeometry(){
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    
    if (settings.contains("windowPosition")){
        QPoint pos = settings.value("windowPosition").toPoint();
        move(pos);
    }
}

void MainWindow::showAboutDialog(){
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About UnfuckMyTimeZoneMath");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(
        "<h2>UnfuckMyTimeZoneMath " APP_VERSION "</h2>"
        "<p>A handy utility to help teams figure out time zone math when trying to schedule meetings and stuff.</p>"
        "<p>Visualize and synchronize times across multiple time zones with ease.</p>"
        "<p><a href='https://github.com/Master-Branch-Software/UnfuckMyTimeZoneMath'>github.com/Master-Branch-Software/UnfuckMyTimeZoneMath</a></p>"
    );
    aboutBox.setIcon(QMessageBox::Information);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #f6fafe;"
        "}"
        "QPushButton {"
        "    background-color: #eef4fa;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px 16px;"
        "    min-width: 70px;"
        "    color: #2a343a;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e7eff5;"
        "}"
    );
    aboutBox.exec();
}

void MainWindow::onWidgetDropped(TimeZoneWidget *target, TimeZoneWidget *source)
{
    int sourceIndex = timeZoneWidgets.indexOf(source);
    int targetIndex = timeZoneWidgets.indexOf(target);

    if (sourceIndex == -1 || targetIndex == -1 || sourceIndex == targetIndex){
        return;
    }

    QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(centralWidget->layout());

    if (!layout){
        return;
    }

    layout->removeWidget(source);
    timeZoneWidgets.move(sourceIndex, targetIndex);
    layout->insertWidget(targetIndex, source);

    isDirty = true;
    updateWindowTitle();
}

void MainWindow::setToCurrentTime(){
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    
    if (!timeZoneWidgets.isEmpty()){
        updateAllWidgets(currentTime, nullptr);
    }
}

void MainWindow::toggleToolBarVisibility(){
    bool isVisible = toolBar->isVisible();
    toolBar->setVisible(!isVisible);
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    settings.setValue("toolBarVisible", !isVisible);
    
    if (toggleToolBarAction){
        toggleToolBarAction->setChecked(!isVisible);
    }
    
    adjustWindowSize();
}

void MainWindow::toggleToolBarTextVisibility(){
    bool showText = true;

    // Determine source of the toggle (toolbar button vs View menu item)
    QAction *senderAction = qobject_cast<QAction*>(sender());

    if (senderAction){
        showText = senderAction->isChecked();
    }
    else if (toolBarTextToggleAction){
        showText = toolBarTextToggleAction->isChecked();
    }
    
    if (showText){
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    else{
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    settings.setValue("toolBarTextVisible", showText);
    
    if (toggleToolBarTextAction){
        toggleToolBarTextAction->setChecked(showText);
    }
    
    if (toolBarTextToggleAction){
        toolBarTextToggleAction->setChecked(showText);
    }
    
    adjustWindowSize();
}

void MainWindow::showSettings(){
    if (!settingsDialog){
        settingsDialog = new SettingsDialog(this);
    }
    
    if (settingsDialog->exec() == QDialog::Accepted){
        for (TimeZoneWidget *widget : timeZoneWidgets){
            widget->reloadSettings();
        }
        
        QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
        bool skyColorEnabled = settings.value("appearance/skyColor", true).toBool();
        if (mainToolBarActions.size() > 6){
            mainToolBarActions[6]->setChecked(skyColorEnabled);
        }
    }
}

void MainWindow::toggleSkyColor(){
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    bool currentState = settings.value("appearance/skyColor", true).toBool();
    bool newState = !currentState;
    
    settings.setValue("appearance/skyColor", newState);
    
    for (TimeZoneWidget *widget : timeZoneWidgets){
        widget->reloadSettings();
    }
    
    if (mainToolBarActions.size() > 6){
        mainToolBarActions[6]->setChecked(newState);
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
#ifdef Q_OS_WIN
    // On Windows, only DoubleClick is emitted for double-click
    if (reason == QSystemTrayIcon::DoubleClick){
        toggleWindowVisibility();
    }
#else
    // On Linux/macOS, Trigger is emitted for single click
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick){
        toggleWindowVisibility();
    }
#endif
}

void MainWindow::toggleWindowVisibility(){
    QTimer::singleShot(0, this, [this]() {
        if (isVisible()){
            hide();
        }
        else{
            setWindowState(Qt::WindowNoState);
            showNormal();
            raise();
            activateWindow();
            
#ifdef Q_OS_LINUX
            // Process events to prevent black window on KDE/Plasma
            QApplication::processEvents();
#endif
        }
        
        updateTrayMenu();
    });
}

void MainWindow::quitApplication(){
    forceQuit = true;
    
    if (localServer){
        localServer->close();
    }
    
    QApplication::quit();
}

void MainWindow::openRecentFileFromTray(){
    QAction *action = qobject_cast<QAction*>(sender());
    
    if (action){
        setWindowState(Qt::WindowNoState);
        showNormal();
        raise();
        activateWindow();
        
#ifdef Q_OS_LINUX
        // Process events to prevent black window on KDE/Plasma
        QApplication::processEvents();
#endif
        
        if (!maybeSave()){
            return;
        }
        
        loadFromFile(action->data().toString());
    }
}

void MainWindow::updateTrayMenu(){
    if (!trayMenu || !trayIcon){
        return;
    }
    
    trayMenu->clear();
    
    QIcon showHideIcon = isVisible()
        ? QIcon(":/toolbar/toolbar-icons/view-hidden.svg")
        : QIcon(":/toolbar/toolbar-icons/view-visible.svg");

    QAction *showAction = trayMenu->addAction(showHideIcon, isVisible() ? "Hide Window" : "Show Window");
    connect(showAction, &QAction::triggered, this, &MainWindow::toggleWindowVisibility);
    
    trayMenu->addSeparator();
    
    trayRecentFilesMenu = trayMenu->addMenu(QIcon(":/toolbar/toolbar-icons/document-open.svg"), "Recent Files");
    
    QStringList recentFiles = getRecentFiles();
    
    if (recentFiles.isEmpty()){
        QAction *noFilesAction = trayRecentFilesMenu->addAction("No Recent Files");
        noFilesAction->setEnabled(false);
    }
    else{
        for (const QString &filename : recentFiles){
            QFileInfo fileInfo(filename);
            QString displayName = fileInfo.completeBaseName();
            QAction *action = trayRecentFilesMenu->addAction(displayName);
            action->setData(filename);
            connect(action, &QAction::triggered, this, &MainWindow::openRecentFileFromTray);
        }
    }
    
    trayMenu->addSeparator();
    
    QAction *newAction = trayMenu->addAction(QIcon(":/toolbar/toolbar-icons/document-new.svg"), "New");
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    
    QAction *openAction = trayMenu->addAction(QIcon(":/toolbar/toolbar-icons/document-open.svg"), "Open...");
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    
    QAction *settingsAction = trayMenu->addAction(QIcon(":/toolbar/toolbar-icons/configure.svg"), "Settings...");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    trayMenu->addSeparator();
    
    QAction *quitAction = trayMenu->addAction(QIcon(":/toolbar/toolbar-icons/application-exit.svg"), "Quit");
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    
    trayIcon->setContextMenu(trayMenu);
}

void MainWindow::handleNewConnection(){
    QLocalSocket *socket = localServer->nextPendingConnection();
    if (socket){
        socket->deleteLater();
        
        setWindowState(Qt::WindowNoState);
        showNormal();
        raise();
        activateWindow();
        
#ifdef Q_OS_LINUX
        // Process events to prevent black window on KDE/Plasma
        QApplication::processEvents();
#endif
    }
}

void MainWindow::copyToClipboard(){
    if (timeZoneWidgets.isEmpty()){
        return;
    }
    
    QStringList clipboardLines;
    
    for (TimeZoneWidget *widget : timeZoneWidgets){
        QString name = widget->getFriendlyName();
        qint64 timestamp = widget->getBaseTimestamp();
        QString timezoneId = widget->getTimeZoneId();
        bool is24Hour = widget->getIs24HourFormat();
        
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone(timezoneId.toUtf8()));
        QString timeFormat = is24Hour ? "HH:mm" : "h:mma";
        QString formattedTime = dateTime.toString(timeFormat);
        
        QString line = QString("%1, %2, %3").arg(name, formattedTime, timezoneId);
        clipboardLines.append(line);
    }
    
    QString clipboardText = clipboardLines.join("\n");
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(clipboardText);
}

void MainWindow::initializeSystemTray(){
    if (trayIcon && trayIcon->isVisible()){
        return;
    }

    if (QSystemTrayIcon::isSystemTrayAvailable()){
        if (trayInitTimer){
            trayInitTimer->stop();
            trayInitTimer->deleteLater();
            trayInitTimer = nullptr;
        }

        trayIcon = new QSystemTrayIcon(this);
        trayMenu = new QMenu(this);

        QIcon trayIconImage(":/icons/icon.png");

        if (trayIconImage.isNull() && QFile::exists("/usr/share/icons/hicolor/256x256/apps/unfuck-my-timezone-math.png")){
            trayIconImage = QIcon("/usr/share/icons/hicolor/256x256/apps/unfuck-my-timezone-math.png");
        }

        if (trayIconImage.isNull()){
            trayIconImage = QIcon::fromTheme("unfuck-my-timezone-math");
        }

        if (trayIconImage.isNull()){
            trayIconImage = QIcon::fromTheme("preferences-system");
        }

        if (trayIconImage.isNull()){
            trayIconImage = QIcon::fromTheme("application-x-executable");
        }

        trayIcon->setIcon(trayIconImage);
        trayIcon->setToolTip("UnfuckMyTimeZoneMath");

        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

        updateTrayMenu();
        trayIcon->show();

        qDebug() << "System tray icon initialized successfully";
    }
    else{
        trayInitAttempts++;

        if (trayInitAttempts <= 30){
            if (!trayInitTimer){
                trayInitTimer = new QTimer(this);
                connect(trayInitTimer, &QTimer::timeout, this, &MainWindow::initializeSystemTray);
            }

            trayInitTimer->start(1000);
            qDebug() << "System tray not available yet, retry attempt" << trayInitAttempts << "of 30";
        }
        else{
            qWarning("System tray is not available after 30 attempts");

            if (trayInitTimer){
                trayInitTimer->stop();
                trayInitTimer->deleteLater();
                trayInitTimer = nullptr;
            }
        }
    }
}

