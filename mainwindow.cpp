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
#include <QStandardPaths>
#include <QDir>
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFrame>
#include <QPixmap>
#include <QComboBox>
#include <QHideEvent>
#include <QToolButton>
#include "copydialog.hpp"

#ifdef Q_OS_MACOS
#include "macos_helper.hpp"
#endif

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent),
    recentFilesCombo(nullptr),
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
    
    QIcon icon = QIcon::fromTheme("ufemtizm");
    if (icon.isNull()){
        icon = QIcon("/usr/share/icons/hicolor/256x256/apps/ufemtizm.png");
    }

    if (!icon.isNull()){
        setWindowIcon(icon);
    }
    
    setupMenuBar();
    setupToolBar();
    
    QSettings settings("Ufemtizm", "Ufemtizm");
    
    localServer = new QLocalServer(this);
    QString serverName = "Ufemtizm_SingleInstance";
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

    // Use the stored list (not the filtered one) so that if the most
    // recently opened file has been deleted we can detect it and warn the
    // user instead of silently loading the next existing entry.
    QStringList storedRecentFiles = getStoredRecentFiles();

    if (!storedRecentFiles.isEmpty()){
        QString lastFile = storedRecentFiles.first();

        if (QFile::exists(lastFile)){
            loadFromFile(lastFile);
        }
        else{
            QMessageBox::warning(this, "File Not Found",
                                QString("The last opened file was not found:\n%1\n\nStarting with a new file.").arg(lastFile));
            removeRecentFile(lastFile);
            updateRecentFilesMenu();
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
    
    recentFilesMenu = fileMenu->addMenu(QIcon(":/toolbar/toolbar-icons/document-open.svg"), "&Recent Files");
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
    
    QAction *settingsAction = fileMenu->addAction(QIcon(":/toolbar/toolbar-icons/configure.svg"), "Settin&gs...");
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
    
    QAction *copySelectAction = editMenu->addAction(QIcon(":/toolbar/toolbar-icons/edit-copy.svg"), "Copy &Selected...");
    copySelectAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    copySelectAction->setToolTip("Choose which timezones to copy (Ctrl+Shift+C)");
    copySelectAction->setStatusTip("Choose which timezones to copy");
    connect(copySelectAction, &QAction::triggered, this, &MainWindow::copyToClipboardDirect);
    
    QMenu *viewMenu = menuBar()->addMenu("&View");
    
    toggleToolBarAction = viewMenu->addAction("Show &Toolbar");
    toggleToolBarAction->setCheckable(true);
    toggleToolBarAction->setChecked(true);
    toggleToolBarAction->setToolTip("Toggle toolbar visibility");
    toggleToolBarAction->setStatusTip("Toggle toolbar visibility");
    connect(toggleToolBarAction, &QAction::triggered, this, &MainWindow::toggleToolBarVisibility);
    
    toggleToolBarTextAction = viewMenu->addAction("Show Toolbar T&ext");
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

    // Recent files drop-down lives in the menu bar's right corner so it
    // stays visible regardless of toolbar width, and so QToolBar's built-in
    // extension chevron can handle toolbar overflow on narrow windows
    // without a custom widget-action getting in its way.
    recentFilesCombo = new QComboBox();
    recentFilesCombo->setToolTip("Current file. Select a recent file to open it.");
    recentFilesCombo->setStatusTip("Current file. Select a recent file to open it.");
    recentFilesCombo->setMinimumWidth(140);
    recentFilesCombo->setMaximumWidth(260);
    recentFilesCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    recentFilesCombo->setPlaceholderText("Untitled");
    recentFilesCombo->setStyleSheet(
        "QComboBox {"
        "    background-color: #eef4fa;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 3px 10px;"
        "    color: #2a343a;"
        "}"
        "QComboBox:hover {"
        "    background-color: #e7eff5;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 4px;"
        "    selection-background-color: #eef4fa;"
        "    selection-color: #2a343a;"
        "    color: #2a343a;"
        "}"
    );
    connect(recentFilesCombo, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::onRecentFilesComboActivated);

    // QMenuBar's corner widget is placed flush against the window edge, so
    // wrap the combo in a container with a right margin to give it some
    // breathing room. Stylesheet margin-right on the combo itself is
    // unreliable inside the menu bar corner.
    QWidget *cornerContainer = new QWidget(menuBar());
    QHBoxLayout *cornerLayout = new QHBoxLayout(cornerContainer);
    cornerLayout->setContentsMargins(0, 0, 10, 0);
    cornerLayout->setSpacing(0);
    cornerLayout->addWidget(recentFilesCombo);
    menuBar()->setCornerWidget(cornerContainer, Qt::TopRightCorner);
}

void MainWindow::setupToolBar(){
    toolBar = addToolBar("Main Toolbar");
    toolBar->setObjectName("MainToolBar");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(22, 22));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // The toolbar's extension chevron (shown when items overflow) is drawn
    // by QStyle using the palette's ButtonText color, not the QSS `color`
    // property. Force a dark ButtonText so the arrow is visible against
    // the white toolbar background.
    QPalette toolBarPalette = toolBar->palette();
    toolBarPalette.setColor(QPalette::ButtonText, QColor(0x2a, 0x34, 0x3a));
    toolBarPalette.setColor(QPalette::WindowText, QColor(0x2a, 0x34, 0x3a));
    toolBar->setPalette(toolBarPalette);

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
        "QToolButton#qt_toolbar_ext_button {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 2px;"
        "    margin: 0px;"
        "    color: #2a343a;"
        "    qproperty-icon: url(:/toolbar/toolbar-icons/overflow-menu.svg);"
        "}"
        "QToolButton#qt_toolbar_ext_button:hover {"
        "    background-color: #eef4fa;"
        "}"
    );

    // The toolbar's overflow chevron (QToolBarExtension, objectName
    // qt_toolbar_ext_button) is constructed inside the QToolBar
    // constructor, so it already exists here. On some Linux styles its
    // built-in arrow primitive renders nearly invisible, so force an
    // explicit SVG icon and icon-only layout on it now.
    QToolButton *extensionButton = toolBar->findChild<QToolButton*>("qt_toolbar_ext_button");

    if (extensionButton){
        extensionButton->setArrowType(Qt::NoArrow);
        extensionButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        extensionButton->setIcon(QIcon(":/toolbar/toolbar-icons/overflow-menu.svg"));
        extensionButton->setIconSize(QSize(18, 18));
        extensionButton->setAutoRaise(false);
    }

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
    QSettings skySettings("Ufemtizm", "Ufemtizm");
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

    QSettings settings("Ufemtizm", "Ufemtizm");
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
    QSettings settings("Ufemtizm", "Ufemtizm");
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
        QSettings settings("Ufemtizm", "Ufemtizm");
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

    // Keep the tray menu's show/hide entry tightly coupled to actual
    // window visibility so it never drifts out of sync regardless of
    // which code path triggered the change.
    updateTrayMenu();

#ifdef Q_OS_MACOS
    // Promote to a regular foreground app so the global menu bar appears
    // while the main window is visible. The app starts as an accessory
    // (LSUIElement) so the dock icon and menu stay hidden while the window
    // is tucked away in the tray.
    setMacosActivationPolicyRegular();
#endif

#ifdef Q_OS_LINUX
    QTimer::singleShot(0, this, [this](){
        update();
        if (centralWidget){
            centralWidget->update();
        }
    });
#endif
}

void MainWindow::hideEvent(QHideEvent *event){
    QMainWindow::hideEvent(event);

    // Close-to-tray and minimize-to-tray both hide the window without firing
    // the force-quit path in closeEvent. Persist the last visible position
    // here so the next launch can restore it.
    saveWindowGeometry();

    // Keep the tray menu's show/hide entry tightly coupled to actual
    // window visibility so it never drifts out of sync regardless of
    // which code path triggered the change.
    updateTrayMenu();

#ifdef Q_OS_MACOS
    // Drop back to accessory policy so the Dock icon and global menu bar
    // disappear while the window is hidden to the system tray.
    setMacosActivationPolicyAccessory();
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

    QSettings settings("Ufemtizm", "Ufemtizm");
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString lastDir = settings.value("fileDialog/lastDirectory", defaultDir).toString();

    if (lastDir.isEmpty() || !QDir(lastDir).exists()){
        lastDir = defaultDir;
    }

    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Configuration");
    dialog.setDirectory(lastDir);
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
            QString selectedFile = files.first();
            settings.setValue("fileDialog/lastDirectory", QFileInfo(selectedFile).absolutePath());
            loadFromFile(selectedFile);
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
    QSettings settings("Ufemtizm", "Ufemtizm");
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString lastDir = settings.value("fileDialog/lastDirectory", defaultDir).toString();

    if (lastDir.isEmpty() || !QDir(lastDir).exists()){
        lastDir = defaultDir;
    }

    QFileDialog dialog(this);
    dialog.setWindowTitle("Save Configuration");
    dialog.setDirectory(lastDir);
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

            settings.setValue("fileDialog/lastDirectory", QFileInfo(filename).absolutePath());
            saveToFile(filename);
        }
    }
}

void MainWindow::openRecentFile(){
    QAction *action = qobject_cast<QAction*>(sender());

    if (!action){
        return;
    }

    QString filename = action->data().toString();

    if (filename.isEmpty()){
        return;
    }

    if (!QFile::exists(filename)){
        warnFileNotFoundAndForget(filename);
        return;
    }

    if (!maybeSave()){
        return;
    }

    loadFromFile(filename);
}

void MainWindow::updateWindowTitle(){
    // Keep the application display name (what Qt appends after the em-dash)
    // in sync with the user-configurable display title from settings. Qt
    // automatically joins windowTitle() and applicationDisplayName() with
    // " \u2014 ", so never include the display name in the window title
    // itself.
    qApp->setApplicationDisplayName(SettingsDialog::effectiveDisplayName());

    QString fileLabel;

    if (currentFilename.isEmpty()){
        // Only show a label for an unsaved document when it is dirty;
        // otherwise fall back to showing just the display name alone.
        if (isDirty){
            fileLabel = QStringLiteral("Untitled");
        }
    }
    else{
        QFileInfo fileInfo(currentFilename);
        fileLabel = fileInfo.completeBaseName();
    }

    // [*] is Qt's placeholder for the modified marker, controlled by
    // setWindowModified(). It expands to "*" when modified, empty otherwise.
    if (!fileLabel.isEmpty()){
        fileLabel += QStringLiteral("[*]");
    }

    setWindowTitle(fileLabel);
    setWindowModified(isDirty);

    // Reflect the currently open file in the toolbar's recent files combo.
    if (recentFilesCombo){
        QSignalBlocker blocker(recentFilesCombo);

        if (currentFilename.isEmpty()){
            recentFilesCombo->setCurrentIndex(-1);
        }
        else{
            int index = recentFilesCombo->findData(currentFilename);

            if (index >= 0){
                recentFilesCombo->setCurrentIndex(index);
            }
            else{
                recentFilesCombo->setCurrentIndex(-1);
            }
        }
    }
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

    // Mirror the recent files list in the menu-bar corner combo. Each item
    // stores the full path in its user data so we can open it when
    // activated.
    if (recentFilesCombo){
        QSignalBlocker blocker(recentFilesCombo);

        recentFilesCombo->clear();

        for (const QString &filename : recentFiles){
            QFileInfo fileInfo(filename);
            recentFilesCombo->addItem(fileInfo.completeBaseName(), filename);
        }

        if (currentFilename.isEmpty()){
            recentFilesCombo->setCurrentIndex(-1);
        }
        else{
            int index = recentFilesCombo->findData(currentFilename);

            if (index >= 0){
                recentFilesCombo->setCurrentIndex(index);
            }
            else{
                recentFilesCombo->setCurrentIndex(-1);
            }
        }
    }

    updateTrayMenu();
}

void MainWindow::onRecentFilesComboActivated(int index){
    if (!recentFilesCombo || index < 0){
        return;
    }

    QString filename = recentFilesCombo->itemData(index).toString();

    if (filename.isEmpty() || filename == currentFilename){
        return;
    }

    if (!QFile::exists(filename)){
        // Snap the combo back to whatever is currently loaded before the
        // warning appears so the UI does not show the missing file as
        // the active selection.
        {
            QSignalBlocker blocker(recentFilesCombo);
            recentFilesCombo->setCurrentIndex(recentFilesCombo->findData(currentFilename));
        }

        warnFileNotFoundAndForget(filename);
        return;
    }

    if (!maybeSave()){
        // User cancelled; restore the combo to reflect the still-current file.
        QSignalBlocker blocker(recentFilesCombo);
        int currentIndex = recentFilesCombo->findData(currentFilename);
        recentFilesCombo->setCurrentIndex(currentIndex);
        return;
    }

    loadFromFile(filename);
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
    QSettings settings("Ufemtizm", "Ufemtizm");
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    
    recentFiles.removeAll(filename);
    recentFiles.prepend(filename);
    
    while (recentFiles.size() > 10){
        recentFiles.removeLast();
    }
    
    settings.setValue("recentFiles", recentFiles);
}

void MainWindow::removeRecentFile(const QString &filename)
{
    QSettings settings("Ufemtizm", "Ufemtizm");
    QStringList recentFiles = settings.value("recentFiles").toStringList();

    if (recentFiles.removeAll(filename) > 0){
        settings.setValue("recentFiles", recentFiles);
    }
}

QStringList MainWindow::getStoredRecentFiles() const
{
    QSettings settings("Ufemtizm", "Ufemtizm");

    return settings.value("recentFiles").toStringList();
}

void MainWindow::warnFileNotFoundAndForget(const QString &filename)
{
    QMessageBox::warning(this, "File Not Found",
                        QString("The file was not found:\n%1\n\nIt has been removed from the recent files list.").arg(filename));

    removeRecentFile(filename);
    updateRecentFilesMenu();
}

QStringList MainWindow::getRecentFiles() const
{
    QSettings settings("Ufemtizm", "Ufemtizm");
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

    int contentWidth = (widgetWidth * widgetCount) + (spacing * (widgetCount - 1)) + margins;
    int contentHeight = 700;

    int menuBarHeight = menuBar()->height();
    int toolBarHeight = toolBar->isVisible() ? toolBar->height() : 0;

    int totalHeight = contentHeight + menuBarHeight + toolBarHeight;

    // Make sure the menu bar (which now hosts the recent-files combo in
    // its right corner) has room for itself, and that the toolbar has room
    // for all its buttons plus its overflow chevron so the chevron isn't
    // clipped by the window edge.
    int menuBarMinWidth = menuBar()->sizeHint().width();
    int toolBarMinWidth = toolBar->isVisible() ? toolBar->sizeHint().width() : 0;
    int totalWidth = std::max({contentWidth, menuBarMinWidth, toolBarMinWidth});

    setFixedSize(totalWidth, totalHeight);
}

void MainWindow::saveWindowGeometry(){
    // Skip minimized state - pos() can return garbage when the window is
    // iconified, which would otherwise overwrite a valid saved position.
    if (isMinimized()){
        return;
    }

    QPoint currentPos = pos();

    // A (0,0) position usually means the window has not been placed yet
    // (e.g. startMinimized without ever showing), so don't overwrite a
    // previously saved value with it.
    if (currentPos.isNull()){
        return;
    }

    QSettings settings("Ufemtizm", "Ufemtizm");
    settings.setValue("windowPosition", currentPos);
}

void MainWindow::restoreWindowGeometry(){
    QSettings settings("Ufemtizm", "Ufemtizm");

    if (!settings.contains("windowPosition")){
        return;
    }

    QPoint savedPos = settings.value("windowPosition").toPoint();
    move(savedPos);
}

void MainWindow::showAboutDialog(){
    QString appName = SettingsDialog::effectiveDisplayName();

    QDialog aboutBox(this);
    aboutBox.setWindowTitle("About " + appName);
    aboutBox.setModal(true);

    // Prefer the bundled resource icon for crispness at 48px. Fall back to
    // the installed theme icon, then the system-installed PNG.
    QPixmap iconPixmap(":/icons/icon.png");
    if (iconPixmap.isNull()){
        iconPixmap = QIcon::fromTheme("ufemtizm").pixmap(48, 48);
    }
    if (iconPixmap.isNull()){
        iconPixmap = QIcon("/usr/share/icons/hicolor/256x256/apps/ufemtizm.png").pixmap(48, 48);
    }
    if (!iconPixmap.isNull()){
        iconPixmap = iconPixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QLabel *iconLabel = new QLabel(&aboutBox);
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(48, 48);
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel(&aboutBox);
    titleLabel->setTextFormat(Qt::RichText);
    titleLabel->setText("<h2 style='margin:0;'>" + appName + " " APP_VERSION "</h2>");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);
    headerLayout->addWidget(iconLabel, 0, Qt::AlignTop);
    headerLayout->addWidget(titleLabel, 1);

    QLabel *descriptionLabel = new QLabel(&aboutBox);
    descriptionLabel->setTextFormat(Qt::RichText);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setText(
        "<p>A handy utility to help teams figure out time zone math when trying to schedule meetings and stuff.</p>"
        "<p>Visualize and synchronize times across multiple time zones with ease.</p>"
    );

    QFrame *separator = new QFrame(&aboutBox);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background-color: #e7eff5; border: none;");

    QLabel *urlLabel = new QLabel(&aboutBox);
    urlLabel->setTextFormat(Qt::RichText);
    urlLabel->setAlignment(Qt::AlignCenter);
    urlLabel->setOpenExternalLinks(true);
    urlLabel->setText("<a href='https://masterbranchsoftware.com'>masterbranchsoftware.com</a>");

    QLabel *copyrightLabel = new QLabel(&aboutBox);
    copyrightLabel->setTextFormat(Qt::RichText);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    copyrightLabel->setText(
        "<span style='color:#6b7a85;font-size:small;'>&copy; 2025 Master Branch Software, LLC</span>"
    );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &aboutBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &aboutBox, &QDialog::accept);

    QVBoxLayout *mainLayout = new QVBoxLayout(&aboutBox);
    mainLayout->setContentsMargins(20, 20, 20, 16);
    mainLayout->setSpacing(12);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addWidget(separator);
    mainLayout->addWidget(urlLabel);
    mainLayout->addWidget(copyrightLabel);
    mainLayout->addSpacing(4);
    mainLayout->addWidget(buttonBox);

    aboutBox.setStyleSheet(
        "QDialog {"
        "    background-color: #f6fafe;"
        "}"
        "QLabel {"
        "    color: #2a343a;"
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
        "}"
        "QPushButton:default:hover {"
        "    background-color: #4135d8;"
        "}"
    );

    aboutBox.setMinimumWidth(440);
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
    
    QSettings settings("Ufemtizm", "Ufemtizm");
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
    
    QSettings settings("Ufemtizm", "Ufemtizm");
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

        updateWindowTitle();

        if (trayIcon){
            trayIcon->setToolTip(SettingsDialog::effectiveDisplayName());
        }

        QSettings settings("Ufemtizm", "Ufemtizm");
        bool skyColorEnabled = settings.value("appearance/skyColor", true).toBool();

        if (mainToolBarActions.size() > 6){
            mainToolBarActions[6]->setChecked(skyColorEnabled);
        }
    }
}

void MainWindow::toggleSkyColor(){
    QSettings settings("Ufemtizm", "Ufemtizm");
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

        // The tray menu is refreshed from showEvent/hideEvent, so no
        // explicit updateTrayMenu() call is needed here.
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

    QAction *aboutTrayAction = trayMenu->addAction("About...");
    connect(aboutTrayAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

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
    
    QSettings settings("Ufemtizm", "Ufemtizm");
    bool skipDialog = settings.value("copy/skipDialog", false).toBool();
    
    if (skipDialog){
        QVector<int> allIndices;

        for (int i = 0; i < timeZoneWidgets.size(); i++){
            allIndices.append(i);
        }

        performCopy(allIndices);

        return;
    }
    
    QStringList tileNames;

    for (TimeZoneWidget *widget : timeZoneWidgets){
        tileNames.append(widget->getFriendlyName());
    }
    
    CopyDialog dialog(tileNames, true, this);

    if (dialog.exec() == QDialog::Accepted){
        if (dialog.dontShowAgain()){
            settings.setValue("copy/skipDialog", true);
        }

        performCopy(dialog.selectedIndices());
    }
}

void MainWindow::copyToClipboardDirect(){
    if (timeZoneWidgets.isEmpty()){
        return;
    }
    
    QStringList tileNames;

    for (TimeZoneWidget *widget : timeZoneWidgets){
        tileNames.append(widget->getFriendlyName());
    }
    
    CopyDialog dialog(tileNames, false, this);

    if (dialog.exec() == QDialog::Accepted){
        performCopy(dialog.selectedIndices());
    }
}

void MainWindow::performCopy(const QVector<int> &indices){
    QStringList clipboardLines;
    
    for (int index : indices){
        if (index < 0 || index >= timeZoneWidgets.size()){
            continue;
        }

        TimeZoneWidget *widget = timeZoneWidgets[index];
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
    
    if (clipboardLines.isEmpty()){
        return;
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

        if (trayIconImage.isNull() && QFile::exists("/usr/share/icons/hicolor/256x256/apps/ufemtizm.png")){
            trayIconImage = QIcon("/usr/share/icons/hicolor/256x256/apps/ufemtizm.png");
        }

        if (trayIconImage.isNull()){
            trayIconImage = QIcon::fromTheme("ufemtizm");
        }

        if (trayIconImage.isNull()){
            trayIconImage = QIcon::fromTheme("preferences-system");
        }

        if (trayIconImage.isNull()){
            trayIconImage = QIcon::fromTheme("application-x-executable");
        }

        trayIcon->setIcon(trayIconImage);
        trayIcon->setToolTip(SettingsDialog::effectiveDisplayName());

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

