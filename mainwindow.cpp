#include "mainwindow.h"
#include "timezonewidget.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      isDirty(false)
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
    
    setupMenuBar();
    
    centralWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(centralWidget);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    setCentralWidget(centralWidget);
    
    setMinimumSize(QSize(280, 720));
    
    addTimeZoneWidget();
    updateWindowTitle();
    isDirty = false;
    
    restoreWindowGeometry();
    
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
    
    QAction *exitAction = fileMenu->addAction("&Quit");
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setToolTip("Exit the application");
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
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

void MainWindow::addTimeZoneWidget()
{
    TimeZoneWidget *widget = new TimeZoneWidget(centralWidget);
    
    connect(widget, &TimeZoneWidget::timeChanged, this, &MainWindow::onTimeChanged);
    connect(widget, &TimeZoneWidget::removeRequested, this, &MainWindow::removeTimeZoneWidget);
    connect(widget, &TimeZoneWidget::widgetModified, this, &MainWindow::onWidgetModified);
    
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
    
    QString filename = QFileDialog::getOpenFileName(this, "Open Configuration",
                                                     QDir::homePath(),
                                                     "YAML Files (*.yaml *.yml)");
    
    if (!filename.isEmpty())
    {
        loadFromFile(filename);
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
    QString filename = QFileDialog::getSaveFileName(this, "Save Configuration",
                                                     QDir::homePath(),
                                                     "YAML Files (*.yaml *.yml)");
    
    if (!filename.isEmpty())
    {
        if (!filename.endsWith(".yaml", Qt::CaseInsensitive) && !filename.endsWith(".yml", Qt::CaseInsensitive))
        {
            filename += ".yaml";
        }
        
        saveToFile(filename);
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
    int minHeight = 700;
    
    resize(totalWidth, minHeight);
    setMinimumSize(totalWidth, minHeight);
    setMaximumWidth(totalWidth);
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
