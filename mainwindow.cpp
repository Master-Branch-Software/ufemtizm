#include "mainwindow.h"
#include "timezonewidget.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      isDirty(false)
{
    setupMenuBar();
    
    centralWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(centralWidget);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    
    setCentralWidget(centralWidget);
    
    setFixedSize(QSize(300, 700));
    
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
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    
    QAction *openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    
    recentFilesMenu = fileMenu->addMenu("Recent Files");
    updateRecentFilesMenu();
    
    fileMenu->addSeparator();
    
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    
    QAction *saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    
    fileMenu->addSeparator();
    
    QAction *addAction = fileMenu->addAction("Add Time &Zone");
    addAction->setShortcut(QKeySequence("Ctrl+A"));
    connect(addAction, &QAction::triggered, this, &MainWindow::addTimeZoneWidget);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("&Quit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
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
    
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Unsaved Changes",
        "Do you want to save your changes?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    
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
    int widgetWidth = 210;
    int widgetCount = timeZoneWidgets.size();
    int spacing = 5;
    
    int totalWidth = (widgetWidth * widgetCount) + (spacing * (widgetCount - 1)) + 10;
    int height = 680;
    
    setFixedSize(totalWidth, height);
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
