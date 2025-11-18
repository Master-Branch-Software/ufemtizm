#include "mainwindow.h"
#include "timezonewidget.h"
#include "settingsdialog.h"
#include "version.h"
#include <QMenuBar>
#include <QStatusBar>
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      isDirty(false),
      trayIcon(nullptr),
      trayMenu(nullptr),
      trayRecentFilesMenu(nullptr),
      settingsDialog(nullptr),
      localServer(nullptr),
      forceQuit(false),
      initialSizeSet(false)
{
    setStyleSheet(
        "QMainWindow {"
        "    background-color: #f5f5f5;"
        "}"
        "QMenuBar {"
        "    background-color: #ffffff;"
        "    border-bottom: 1px solid #e0e0e0;"
        "    padding: 4px;"
        "}"
        "QMenuBar::item {"
        "    background-color: transparent;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #e3f2fd;"
        "}"
        "QMenu {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    padding: 4px;"
        "}"
        "QMenu::item {"
        "    padding: 8px 24px 8px 12px;"
        "    border-radius: 4px;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #e3f2fd;"
        "}"
        "QMenu::separator {"
        "    height: 1px;"
        "    background-color: #e0e0e0;"
        "    margin: 4px 8px;"
        "}"
    );
    
    QIcon icon = QIcon::fromTheme("unfuck-my-timezone-math");
    if (icon.isNull())
    {
        icon = QIcon("/usr/share/icons/hicolor/256x256/apps/unfuck-my-timezone-math.png");
    }
    if (!icon.isNull())
    {
        setWindowIcon(icon);
    }
    
    setupMenuBar();
    setupToolBar();
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    
    localServer = new QLocalServer(this);
    QString serverName = "UnfuckMyTimeZoneMath_SingleInstance";
    QLocalServer::removeServer(serverName);
    if (localServer->listen(serverName))
    {
        connect(localServer, &QLocalServer::newConnection, this, &MainWindow::handleNewConnection);
    }
    
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        trayIcon = new QSystemTrayIcon(this);
        trayMenu = new QMenu(this);
        
        QIcon trayIconImage(":/icons/icon.png");
        trayIcon->setIcon(trayIconImage);
        trayIcon->setToolTip("UnfuckMyTimeZoneMath");
        
        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
        
        updateTrayMenu();
        trayIcon->show();
    }
    else
    {
        qWarning("System tray is not available on this system");
    }
    
    setAttribute(Qt::WA_QuitOnClose, false);
    
    centralWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(centralWidget);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    setCentralWidget(centralWidget);
    
    addTimeZoneWidget();
    updateWindowTitle();
    isDirty = false;
    
    QStringList recentFiles = getRecentFiles();
    
    if (!recentFiles.isEmpty())
    {
        QString lastFile = recentFiles.first();
        
        if (QFile::exists(lastFile))
        {
            loadFromFile(lastFile);
        }
        else
        {
            QMessageBox::warning(this, "File Not Found",
                                QString("The last opened file was not found:\n%1\n\nStarting with a new file.").arg(lastFile));
        }
    }
    
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    QAction *newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    newAction->setToolTip("Create a new timezone configuration");
    newAction->setStatusTip("Create a new timezone configuration");
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    
    QAction *openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);
    openAction->setToolTip("Open an existing timezone configuration");
    openAction->setStatusTip("Open an existing timezone configuration");
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    
    recentFilesMenu = fileMenu->addMenu("📂 Recent Files");
    updateRecentFilesMenu();
    
    fileMenu->addSeparator();
    
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setToolTip("Save the current configuration");
    saveAction->setStatusTip("Save the current configuration");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    
    QAction *saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setToolTip("Save the configuration with a new name");
    saveAsAction->setStatusTip("Save the configuration with a new name");
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    
    fileMenu->addSeparator();
    
    QAction *addAction = fileMenu->addAction("➕ Add Time &Zone");
    addAction->setShortcut(QKeySequence("Ctrl+T"));
    addAction->setToolTip("Add a new timezone widget (Ctrl+T)");
    addAction->setStatusTip("Add a new timezone widget");
    connect(addAction, &QAction::triggered, this, &MainWindow::addTimeZoneWidget);
    
    fileMenu->addSeparator();
    
    QAction *currentTimeAction = fileMenu->addAction("🕐 Set to Current Time");
    currentTimeAction->setShortcut(QKeySequence("Alt+T"));
    currentTimeAction->setToolTip("Set all timezones to current time (Alt+T)");
    currentTimeAction->setStatusTip("Set all timezones to current time");
    connect(currentTimeAction, &QAction::triggered, this, &MainWindow::setToCurrentTime);
    
    fileMenu->addSeparator();
    
    QAction *settingsAction = fileMenu->addAction("⚙️ &Settings...");
    settingsAction->setToolTip("Configure application settings");
    settingsAction->setStatusTip("Configure application settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("&Quit");
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setToolTip("Exit the application");
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    
    QAction *copyAction = editMenu->addAction("📋 &Copy");
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
    
    statusBar()->setStyleSheet(
        "QStatusBar {"
        "    background-color: #ffffff;"
        "    border-top: 1px solid #e0e0e0;"
        "    color: #757575;"
        "    font-size: 11px;"
        "}"
    );
    statusBar()->showMessage("Ready");
}

void MainWindow::setupToolBar()
{
    toolBar = addToolBar("Main Toolbar");
    toolBar->setObjectName("MainToolBar");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(22, 22));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setStyleSheet(
        "QToolBar {"
        "    background-color: #ffffff;"
        "    border-bottom: 1px solid #e0e0e0;"
        "    spacing: 4px;"
        "    padding: 4px;"
        "}"
        "QToolButton {"
        "    background-color: transparent;"
        "    border: 1px solid transparent;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    margin: 2px;"
        "}"
        "QToolButton:hover {"
        "    background-color: #e3f2fd;"
        "    border: 1px solid #2196f3;"
        "}"
        "QToolButton:pressed {"
        "    background-color: #bbdefb;"
        "}"
    );
    
    QAction *newAction = toolBar->addAction("📄 New");
    newAction->setToolTip("Create a new timezone configuration (Ctrl+N)");
    newAction->setStatusTip("Create a new timezone configuration");
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    mainToolBarActions.append(newAction);
    
    QAction *openAction = toolBar->addAction("📂 Open");
    openAction->setToolTip("Open an existing timezone configuration (Ctrl+O)");
    openAction->setStatusTip("Open an existing timezone configuration");
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    mainToolBarActions.append(openAction);
    
    QAction *saveAction = toolBar->addAction("💾 Save");
    saveAction->setToolTip("Save the current configuration (Ctrl+S)");
    saveAction->setStatusTip("Save the current configuration");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    mainToolBarActions.append(saveAction);
    
    toolBar->addSeparator();
    
    QAction *copyAction = toolBar->addAction("📋 Copy");
    copyAction->setToolTip("Copy timezone information to clipboard (Ctrl+C)");
    copyAction->setStatusTip("Copy timezone information to clipboard");
    connect(copyAction, &QAction::triggered, this, &MainWindow::copyToClipboard);
    mainToolBarActions.append(copyAction);
    
    QAction *addAction = toolBar->addAction("➕ Add Zone");
    addAction->setToolTip("Add a new timezone widget (Ctrl+T)");
    addAction->setStatusTip("Add a new timezone widget");
    connect(addAction, &QAction::triggered, this, &MainWindow::addTimeZoneWidget);
    mainToolBarActions.append(addAction);
    
    QAction *currentTimeAction = toolBar->addAction("🕐 Current Time");
    currentTimeAction->setToolTip("Set all timezones to current time (Alt+T)");
    currentTimeAction->setStatusTip("Set all timezones to current time");
    connect(currentTimeAction, &QAction::triggered, this, &MainWindow::setToCurrentTime);
    mainToolBarActions.append(currentTimeAction);
    
    toolBar->addSeparator();
    
    QAction *settingsToolBarAction = toolBar->addAction("⚙️ Settings");
    settingsToolBarAction->setToolTip("Configure application settings");
    settingsToolBarAction->setStatusTip("Configure application settings");
    connect(settingsToolBarAction, &QAction::triggered, this, &MainWindow::showSettings);
    mainToolBarActions.append(settingsToolBarAction);
    
    toolBar->addSeparator();
    
    toolBarTextToggleAction = toolBar->addAction("🔤");
    toolBarTextToggleAction->setCheckable(true);
    toolBarTextToggleAction->setChecked(true);
    toolBarTextToggleAction->setToolTip("Toggle toolbar text labels");
    toolBarTextToggleAction->setStatusTip("Toggle toolbar text labels");
    connect(toolBarTextToggleAction, &QAction::triggered, this, &MainWindow::toggleToolBarTextVisibility);
    
    toolBarHideAction = toolBar->addAction("✖️");
    toolBarHideAction->setToolTip("Hide toolbar");
    toolBarHideAction->setStatusTip("Hide toolbar");
    connect(toolBarHideAction, &QAction::triggered, this, &MainWindow::toggleToolBarVisibility);
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    bool toolBarVisible = settings.value("toolBarVisible", true).toBool();
    toolBar->setVisible(toolBarVisible);
    
    bool toolBarTextVisible = settings.value("toolBarTextVisible", true).toBool();
    
    if (!toolBarTextVisible)
    {
        mainToolBarActions[0]->setText("📄");
        mainToolBarActions[1]->setText("📂");
        mainToolBarActions[2]->setText("💾");
        mainToolBarActions[3]->setText("📋");
        mainToolBarActions[4]->setText("➕");
        mainToolBarActions[5]->setText("🕐");
        mainToolBarActions[6]->setText("⚙️");
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    else
    {
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    
    if (toggleToolBarAction)
    {
        toggleToolBarAction->setChecked(toolBarVisible);
    }
    
    if (toggleToolBarTextAction)
    {
        toggleToolBarTextAction->setChecked(toolBarTextVisible);
    }
    
    if (toolBarTextToggleAction)
    {
        toolBarTextToggleAction->setChecked(toolBarTextVisible);
    }
}

void MainWindow::addTimeZoneWidget()
{
    TimeZoneWidget *widget = new TimeZoneWidget(centralWidget);
    
    connect(widget, &TimeZoneWidget::timeChanged, this, &MainWindow::onTimeChanged);
    connect(widget, &TimeZoneWidget::removeRequested, this, &MainWindow::removeTimeZoneWidget);
    connect(widget, &TimeZoneWidget::widgetModified, this, &MainWindow::onWidgetModified);
    connect(widget, &TimeZoneWidget::dropReceived, this, &MainWindow::onWidgetDropped);
    
    if (!timeZoneWidgets.isEmpty())
    {
        widget->setBaseTimestamp(timeZoneWidgets.first()->getBaseTimestamp());
        widget->setIs24HourFormat(timeZoneWidgets.first()->getIs24HourFormat());
    }
    
    timeZoneWidgets.append(widget);
    centralWidget->layout()->addWidget(widget);
    
    widget->selectName();
    
    adjustWindowSize();
    
    isDirty = true;
    updateWindowTitle();
    
    statusBar()->showMessage("Timezone added", 2000);
}

void MainWindow::removeTimeZoneWidget(TimeZoneWidget *widget)
{
    if (timeZoneWidgets.size() <= 1)
    {
        return;
    }
    
    timeZoneWidgets.removeOne(widget);
    widget->deleteLater();
    
    adjustWindowSize();
    
    isDirty = true;
    updateWindowTitle();
}

void MainWindow::onTimeChanged(qint64 baseTimestamp)
{
    TimeZoneWidget *source = qobject_cast<TimeZoneWidget*>(sender());
    updateAllWidgets(baseTimestamp, source);
}

void MainWindow::updateAllWidgets(qint64 baseTimestamp, TimeZoneWidget *source)
{
    for (TimeZoneWidget *widget : timeZoneWidgets)
    {
        if (widget != source)
        {
            widget->setBaseTimestamp(baseTimestamp);
        }
    }
}

void MainWindow::onWidgetModified()
{
    isDirty = true;
    updateWindowTitle();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    bool closeToTray = settings.value("systemTray/closeToTray", true).toBool();
    
    if (!forceQuit && closeToTray && trayIcon && trayIcon->isVisible())
    {
        hide();
        event->ignore();
        return;
    }
    
    if (maybeSave())
    {
        saveWindowGeometry();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
        bool minimizeToTray = settings.value("systemTray/minimizeToTray", true).toBool();
        
        if (isMinimized() && minimizeToTray && trayIcon && trayIcon->isVisible())
        {
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
    
    if (!initialSizeSet)
    {
        initialSizeSet = true;
        adjustWindowSize();
        restoreWindowGeometry();
    }
}

void MainWindow::newFile()
{
    if (!maybeSave())
    {
        return;
    }
    
    while (timeZoneWidgets.size() > 1)
    {
        TimeZoneWidget *widget = timeZoneWidgets.last();
        timeZoneWidgets.removeLast();
        widget->deleteLater();
    }
    
    if (!timeZoneWidgets.isEmpty())
    {
        timeZoneWidgets.first()->setFriendlyName("My Wonderful Self");
    }
    
    adjustWindowSize();
    
    currentFilename.clear();
    isDirty = false;
    updateWindowTitle();
}

void MainWindow::openFile()
{
    if (!maybeSave())
    {
        return;
    }
    
    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Configuration");
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("YAML Files (*.yaml *.yml)");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setStyleSheet(
        "QFileDialog {"
        "    background-color: #ffffff;"
        "}"
        "QDialog {"
        "    background-color: #ffffff;"
        "}"
        "QPushButton {"
        "    background-color: #f5f5f5;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e3f2fd;"
        "    border: 1px solid #2196f3;"
        "}"
        "QPushButton:default {"
        "    background-color: #2196f3;"
        "    color: #ffffff;"
        "    border: 1px solid #1976d2;"
        "}"
        "QPushButton:default:hover {"
        "    background-color: #1976d2;"
        "}"
        "QListView, QTreeView {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 4px;"
        "}"
        "QListView::item:hover, QTreeView::item:hover {"
        "    background-color: #f5f5f5;"
        "}"
        "QListView::item:selected, QTreeView::item:selected {"
        "    background-color: #e3f2fd;"
        "    color: #000000;"
        "}"
        "QLineEdit {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid #2196f3;"
        "}"
        "QComboBox {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "}"
    );
    
    if (dialog.exec() == QDialog::Accepted)
    {
        QStringList files = dialog.selectedFiles();
        if (!files.isEmpty())
        {
            loadFromFile(files.first());
        }
    }
}

void MainWindow::saveFile()
{
    if (currentFilename.isEmpty())
    {
        saveFileAs();
    }
    else
    {
        saveToFile(currentFilename);
    }
}

void MainWindow::saveFileAs()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle("Save Configuration");
    dialog.setDirectory(QDir::homePath());
    dialog.setNameFilter("YAML Files (*.yaml *.yml)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("yaml");
    dialog.setStyleSheet(
        "QFileDialog {"
        "    background-color: #ffffff;"
        "}"
        "QDialog {"
        "    background-color: #ffffff;"
        "}"
        "QPushButton {"
        "    background-color: #f5f5f5;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e3f2fd;"
        "    border: 1px solid #2196f3;"
        "}"
        "QPushButton:default {"
        "    background-color: #2196f3;"
        "    color: #ffffff;"
        "    border: 1px solid #1976d2;"
        "}"
        "QPushButton:default:hover {"
        "    background-color: #1976d2;"
        "}"
        "QListView, QTreeView {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 4px;"
        "}"
        "QListView::item:hover, QTreeView::item:hover {"
        "    background-color: #f5f5f5;"
        "}"
        "QListView::item:selected, QTreeView::item:selected {"
        "    background-color: #e3f2fd;"
        "    color: #000000;"
        "}"
        "QLineEdit {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid #2196f3;"
        "}"
        "QComboBox {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "}"
    );
    
    if (dialog.exec() == QDialog::Accepted)
    {
        QStringList files = dialog.selectedFiles();
        if (!files.isEmpty())
        {
            QString filename = files.first();
            if (!filename.endsWith(".yaml", Qt::CaseInsensitive) && !filename.endsWith(".yml", Qt::CaseInsensitive))
            {
                filename += ".yaml";
            }
            saveToFile(filename);
        }
    }
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction*>(sender());
    
    if (action)
    {
        if (!maybeSave())
        {
            return;
        }
        
        loadFromFile(action->data().toString());
    }
}

void MainWindow::updateWindowTitle()
{
    QString title;
    
    if (currentFilename.isEmpty())
    {
        title = "Untitled";
    }
    else
    {
        QFileInfo fileInfo(currentFilename);
        title = fileInfo.completeBaseName();
    }
    
    if (isDirty)
    {
        title += " *";
    }
    
    title += " - UnfuckMyTimeZoneMath";
    setWindowTitle(title);
}

void MainWindow::updateRecentFilesMenu()
{
    recentFilesMenu->clear();
    
    QStringList recentFiles = getRecentFiles();
    
    if (recentFiles.isEmpty())
    {
        QAction *noFilesAction = recentFilesMenu->addAction("No Recent Files");
        noFilesAction->setEnabled(false);
    }
    else
    {
        for (const QString &filename : recentFiles)
        {
            QFileInfo fileInfo(filename);
            QString displayName = fileInfo.completeBaseName();
            QAction *action = recentFilesMenu->addAction(displayName);
            action->setData(filename);
            connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
        }
    }
    
    updateTrayMenu();
}

bool MainWindow::maybeSave()
{
    if (!isDirty)
    {
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
        "    background-color: #ffffff;"
        "}"
        "QPushButton {"
        "    background-color: #f5f5f5;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 6px 16px;"
        "    min-width: 70px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e3f2fd;"
        "    border: 1px solid #2196f3;"
        "}"
        "QPushButton:default {"
        "    background-color: #2196f3;"
        "    color: #ffffff;"
        "    border: 1px solid #1976d2;"
        "}"
        "QPushButton:default:hover {"
        "    background-color: #1976d2;"
        "}"
    );
    
    int reply = msgBox.exec();
    
    if (reply == QMessageBox::Save)
    {
        saveFile();
        return !isDirty;
    }
    else if (reply == QMessageBox::Discard)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool MainWindow::saveToFile(const QString &filename)
{
    QFile file(filename);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Could not save file: " + file.errorString());
        return false;
    }
    
    QTextStream out(&file);
    out << "timezones:\n";
    
    for (TimeZoneWidget *widget : timeZoneWidgets)
    {
        out << "  - name: \"" << widget->getFriendlyName() << "\"\n";
        out << "    timezone: \"" << widget->getTimeZoneId() << "\"\n";
        out << "    format24hour: " << (widget->getIs24HourFormat() ? "true" : "false") << "\n";
    }
    
    file.close();
    
    setCurrentFile(filename);
    isDirty = false;
    updateWindowTitle();
    
    QFileInfo fileInfo(filename);
    statusBar()->showMessage(QString("Saved: %1").arg(fileInfo.fileName()), 3000);
    
    return true;
}

bool MainWindow::loadFromFile(const QString &filename)
{
    QFile file(filename);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Could not open file: " + file.errorString());
        return false;
    }
    
    while (!timeZoneWidgets.isEmpty())
    {
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
    
    while (!in.atEnd())
    {
        line = in.readLine().trimmed();
        
        if (line == "timezones:")
        {
            inTimezones = true;
            continue;
        }
        
        if (inTimezones && line.startsWith("- name:"))
        {
            if (inEntry && !currentName.isEmpty())
            {
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
    
    if (inEntry && !currentName.isEmpty())
    {
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
    
    if (timeZoneWidgets.isEmpty())
    {
        addTimeZoneWidget();
    }
    else
    {
        qint64 currentTime = QDateTime::currentSecsSinceEpoch();
        
        for (TimeZoneWidget *widget : timeZoneWidgets)
        {
            widget->setBaseTimestamp(currentTime);
        }
        
        adjustWindowSize();
    }
    
    file.close();
    
    setCurrentFile(filename);
    isDirty = false;
    updateWindowTitle();
    
    QFileInfo fileInfo(filename);
    statusBar()->showMessage(QString("Loaded: %1 (%2 timezone%3)")
        .arg(fileInfo.fileName())
        .arg(timeZoneWidgets.size())
        .arg(timeZoneWidgets.size() != 1 ? "s" : ""), 3000);
    
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
    
    while (recentFiles.size() > 10)
    {
        recentFiles.removeLast();
    }
    
    settings.setValue("recentFiles", recentFiles);
}

QStringList MainWindow::getRecentFiles() const
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    
    QStringList existingFiles;
    
    for (const QString &filename : recentFiles)
    {
        if (QFile::exists(filename))
        {
            existingFiles.append(filename);
        }
    }
    
    return existingFiles;
}

void MainWindow::adjustWindowSize()
{
    int widgetWidth = 240;
    int widgetCount = timeZoneWidgets.size();
    int spacing = 12;
    int margins = 32;
    
    int totalWidth = (widgetWidth * widgetCount) + (spacing * (widgetCount - 1)) + margins;
    int contentHeight = 700;
    
    int menuBarHeight = menuBar()->height();
    int toolBarHeight = toolBar->isVisible() ? toolBar->height() : 0;
    int statusBarHeight = statusBar()->height();
    
    int totalHeight = contentHeight + menuBarHeight + toolBarHeight + statusBarHeight;
    
    setFixedSize(totalWidth, totalHeight);
}

void MainWindow::saveWindowGeometry()
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    settings.setValue("windowPosition", pos());
}

void MainWindow::restoreWindowGeometry()
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    
    if (settings.contains("windowPosition"))
    {
        QPoint pos = settings.value("windowPosition").toPoint();
        move(pos);
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About UnfuckMyTimeZoneMath");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(
        "<h2>UnfuckMyTimeZoneMath " APP_VERSION "</h2>"
        "<p>A handy utility to help teams figure out time zone math when trying to schedule meetings and stuff.</p>"
        "<p>Visualize and synchronize times across multiple time zones with ease.</p>"
        "<p><a href='https://github.com/RayParkerBassPlayer/UnfuckMyTimeZoneMath'>github.com/RayParkerBassPlayer/UnfuckMyTimeZoneMath</a></p>"
    );
    aboutBox.setIcon(QMessageBox::Information);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #ffffff;"
        "}"
        "QPushButton {"
        "    background-color: #f5f5f5;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 6px 16px;"
        "    min-width: 70px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e3f2fd;"
        "    border: 1px solid #2196f3;"
        "}"
    );
    aboutBox.exec();
}

void MainWindow::onWidgetDropped(TimeZoneWidget *target, TimeZoneWidget *source)
{
    int sourceIndex = timeZoneWidgets.indexOf(source);
    int targetIndex = timeZoneWidgets.indexOf(target);

    if (sourceIndex == -1 || targetIndex == -1 || sourceIndex == targetIndex)
    {
        return;
    }

    QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(centralWidget->layout());

    if (!layout)
    {
        return;
    }

    layout->removeWidget(source);
    timeZoneWidgets.move(sourceIndex, targetIndex);
    layout->insertWidget(targetIndex, source);

    isDirty = true;
    updateWindowTitle();
    
    statusBar()->showMessage("Widget reordered", 2000);
}

void MainWindow::setToCurrentTime()
{
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    
    if (!timeZoneWidgets.isEmpty())
    {
        updateAllWidgets(currentTime, nullptr);
        statusBar()->showMessage("Set to current time", 2000);
    }
}

void MainWindow::toggleToolBarVisibility()
{
    bool isVisible = toolBar->isVisible();
    toolBar->setVisible(!isVisible);
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    settings.setValue("toolBarVisible", !isVisible);
    
    if (toggleToolBarAction)
    {
        toggleToolBarAction->setChecked(!isVisible);
    }
    
    adjustWindowSize();
    
    statusBar()->showMessage((!isVisible ? "Toolbar shown" : "Toolbar hidden"), 2000);
}

void MainWindow::toggleToolBarTextVisibility()
{
    bool showText = toolBarTextToggleAction->isChecked();
    
    if (showText)
    {
        mainToolBarActions[0]->setText("📄 New");
        mainToolBarActions[1]->setText("📂 Open");
        mainToolBarActions[2]->setText("💾 Save");
        mainToolBarActions[3]->setText("📋 Copy");
        mainToolBarActions[4]->setText("➕ Add Zone");
        mainToolBarActions[5]->setText("🕐 Current Time");
        mainToolBarActions[6]->setText("⚙️ Settings");
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    else
    {
        mainToolBarActions[0]->setText("📄");
        mainToolBarActions[1]->setText("📂");
        mainToolBarActions[2]->setText("💾");
        mainToolBarActions[3]->setText("📋");
        mainToolBarActions[4]->setText("➕");
        mainToolBarActions[5]->setText("🕐");
        mainToolBarActions[6]->setText("⚙️");
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    settings.setValue("toolBarTextVisible", showText);
    
    if (toggleToolBarTextAction)
    {
        toggleToolBarTextAction->setChecked(showText);
    }
    
    if (toolBarTextToggleAction)
    {
        toolBarTextToggleAction->setChecked(showText);
    }
    
    adjustWindowSize();
    
    statusBar()->showMessage((showText ? "Toolbar text shown" : "Toolbar text hidden"), 2000);
}

void MainWindow::showSettings()
{
    if (!settingsDialog)
    {
        settingsDialog = new SettingsDialog(this);
    }
    
    if (settingsDialog->exec() == QDialog::Accepted)
    {
        statusBar()->showMessage("Settings updated", 2000);
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    // Update menu to reflect current window state when user clicks tray icon
    if (reason == QSystemTrayIcon::Context || reason == QSystemTrayIcon::Trigger)
    {
        updateTrayMenu();
    }
}

void MainWindow::toggleWindowVisibility()
{
    QTimer::singleShot(0, this, [this]() {
        if (isVisible())
        {
            hide();
        }
        else
        {
            setWindowState(Qt::WindowNoState);
            showNormal();
            raise();
            activateWindow();
        }
    });
}

void MainWindow::quitApplication()
{
    forceQuit = true;
    
    if (localServer)
    {
        localServer->close();
    }
    
    QApplication::quit();
}

void MainWindow::openRecentFileFromTray()
{
    QAction *action = qobject_cast<QAction*>(sender());
    
    if (action)
    {
        setWindowState(Qt::WindowNoState);
        showNormal();
        raise();
        activateWindow();
        
        if (!maybeSave())
        {
            return;
        }
        
        loadFromFile(action->data().toString());
    }
}

void MainWindow::updateTrayMenu()
{
    if (!trayMenu || !trayIcon)
    {
        return;
    }
    
    trayMenu->clear();
    
    QAction *showAction = trayMenu->addAction(isVisible() ? "🙈 Hide Window" : "👁️ Show Window");
    connect(showAction, &QAction::triggered, this, &MainWindow::toggleWindowVisibility);
    
    trayMenu->addSeparator();
    
    trayRecentFilesMenu = trayMenu->addMenu("📂 Recent Files");
    
    QStringList recentFiles = getRecentFiles();
    
    if (recentFiles.isEmpty())
    {
        QAction *noFilesAction = trayRecentFilesMenu->addAction("No Recent Files");
        noFilesAction->setEnabled(false);
    }
    else
    {
        for (const QString &filename : recentFiles)
        {
            QFileInfo fileInfo(filename);
            QString displayName = fileInfo.completeBaseName();
            QAction *action = trayRecentFilesMenu->addAction(displayName);
            action->setData(filename);
            connect(action, &QAction::triggered, this, &MainWindow::openRecentFileFromTray);
        }
    }
    
    trayMenu->addSeparator();
    
    QAction *newAction = trayMenu->addAction("📄 New");
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    
    QAction *openAction = trayMenu->addAction("📂 Open...");
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    
    QAction *settingsAction = trayMenu->addAction("⚙️ Settings...");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    trayMenu->addSeparator();
    
    QAction *quitAction = trayMenu->addAction("❌ Quit");
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    
    trayIcon->setContextMenu(trayMenu);
}

void MainWindow::handleNewConnection()
{
    QLocalSocket *socket = localServer->nextPendingConnection();
    if (socket)
    {
        socket->deleteLater();
        
        setWindowState(Qt::WindowNoState);
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::copyToClipboard()
{
    if (timeZoneWidgets.isEmpty())
    {
        return;
    }
    
    QStringList clipboardLines;
    
    for (TimeZoneWidget *widget : timeZoneWidgets)
    {
        QString name = widget->getFriendlyName();
        qint64 timestamp = widget->getBaseTimestamp();
        QString timezoneId = widget->getTimeZoneId();
        
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone(timezoneId.toUtf8()));
        QString formattedTime = dateTime.toString("h:mma");
        
        QString line = QString("%1, %2, %3").arg(name, formattedTime, timezoneId);
        clipboardLines.append(line);
    }
    
    QString clipboardText = clipboardLines.join("\n");
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(clipboardText);
    
    statusBar()->showMessage(QString("Copied %1 timezone%2 to clipboard")
        .arg(timeZoneWidgets.size())
        .arg(timeZoneWidgets.size() != 1 ? "s" : ""), 2000);
}

