#include "settingsdialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setModal(true);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QGroupBox *trayGroup = new QGroupBox("System Tray");
    QVBoxLayout *trayLayout = new QVBoxLayout();
    
    minimizeToTrayCheckBox = new QCheckBox("Minimize to system tray");
    minimizeToTrayCheckBox->setToolTip("Hide the window in the system tray when minimized");
    trayLayout->addWidget(minimizeToTrayCheckBox);
    
    closeToTrayCheckBox = new QCheckBox("Close to system tray");
    closeToTrayCheckBox->setToolTip("Keep the application running in the system tray when closed");
    trayLayout->addWidget(closeToTrayCheckBox);
    
    startMinimizedCheckBox = new QCheckBox("Start minimized");
    startMinimizedCheckBox->setToolTip("Start the application minimized in the system tray");
    trayLayout->addWidget(startMinimizedCheckBox);
    
    runAtLoginCheckBox = new QCheckBox("Run at login");
    runAtLoginCheckBox->setToolTip("Automatically start the application when you log in");
    trayLayout->addWidget(runAtLoginCheckBox);
    
    trayGroup->setLayout(trayLayout);
    mainLayout->addWidget(trayGroup);
    
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    connect(runAtLoginCheckBox, &QCheckBox::toggled, [this](bool checked) {
        if (checked)
        {
            startMinimizedCheckBox->setEnabled(true);
            startMinimizedCheckBox->setChecked(true);
        }
    });
    
    setStyleSheet(
        "QDialog {"
        "    background-color: #ffffff;"
        "}"
        "QGroupBox {"
        "    background-color: #f5f5f5;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    margin-top: 12px;"
        "    padding: 12px;"
        "    font-weight: bold;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    padding: 0 8px;"
        "    background-color: #ffffff;"
        "}"
        "QCheckBox {"
        "    spacing: 8px;"
        "    padding: 4px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 18px;"
        "    height: 18px;"
        "    border: 2px solid #bdbdbd;"
        "    border-radius: 4px;"
        "    background-color: #ffffff;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: #2196f3;"
        "    border-color: #2196f3;"
        "    image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNNiAxMUwzIDhMMiA5bDQgNCA4LThMMTMgMiA2IDExeiIgZmlsbD0iI2ZmZiIvPjwvc3ZnPg==);"
        "}"
        "QCheckBox::indicator:disabled {"
        "    background-color: #f5f5f5;"
        "    border-color: #e0e0e0;"
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
    
    loadSettings();
    
    setMinimumWidth(400);
}

SettingsDialog::~SettingsDialog()
{
}

bool SettingsDialog::showInTaskBar() const
{
    return true;
}

bool SettingsDialog::minimizeToTray() const
{
    return minimizeToTrayCheckBox->isChecked();
}

bool SettingsDialog::closeToTray() const
{
    return closeToTrayCheckBox->isChecked();
}

bool SettingsDialog::startMinimized() const
{
    return startMinimizedCheckBox->isChecked();
}

bool SettingsDialog::runAtLogin() const
{
    return runAtLoginCheckBox->isChecked();
}

void SettingsDialog::setShowInTaskBar(bool show)
{
    Q_UNUSED(show);
}

void SettingsDialog::setMinimizeToTray(bool minimize)
{
    minimizeToTrayCheckBox->setChecked(minimize);
}

void SettingsDialog::setCloseToTray(bool close)
{
    closeToTrayCheckBox->setChecked(close);
}

void SettingsDialog::setStartMinimized(bool start)
{
    startMinimizedCheckBox->setChecked(start);
}

void SettingsDialog::setRunAtLogin(bool run)
{
    runAtLoginCheckBox->setChecked(run);
}

void SettingsDialog::onAccepted()
{
    saveSettings();
    accept();
}

void SettingsDialog::loadSettings()
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    
    bool minimizeTray = settings.value("systemTray/minimizeToTray", true).toBool();
    bool closeTray = settings.value("systemTray/closeToTray", true).toBool();
    bool startMin = settings.value("systemTray/startMinimized", false).toBool();
    bool runLogin = settings.value("systemTray/runAtLogin", false).toBool();
    
    minimizeToTrayCheckBox->setChecked(minimizeTray);
    closeToTrayCheckBox->setChecked(closeTray);
    startMinimizedCheckBox->setChecked(startMin);
    runAtLoginCheckBox->setChecked(runLogin);
}

void SettingsDialog::saveSettings()
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    
    settings.setValue("systemTray/minimizeToTray", minimizeToTrayCheckBox->isChecked());
    settings.setValue("systemTray/closeToTray", closeToTrayCheckBox->isChecked());
    settings.setValue("systemTray/startMinimized", startMinimizedCheckBox->isChecked());
    settings.setValue("systemTray/runAtLogin", runAtLoginCheckBox->isChecked());
    
    setupAutostart(runAtLoginCheckBox->isChecked());
}

void SettingsDialog::setupAutostart(bool enable)
{
#ifdef Q_OS_LINUX
    QString autostartPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir autostartDir(autostartPath);
    
    if (!autostartDir.exists())
    {
        autostartDir.mkpath(".");
    }
    
    QString desktopFilePath = autostartPath + "/unfuck-my-timezone-math.desktop";
    
    if (enable)
    {
        QFile desktopFile(desktopFilePath);
        if (desktopFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&desktopFile);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=Unfuck My TimeZone Math\n";
            out << "Comment=A timezone conversion utility\n";
            out << "Exec=" << QCoreApplication::applicationFilePath() << "\n";
            out << "Icon=unfuck-my-timezone-math\n";
            out << "Terminal=false\n";
            out << "Categories=Utility;\n";
            out << "X-GNOME-Autostart-enabled=true\n";
            desktopFile.close();
        }
    }
    else
    {
        QFile::remove(desktopFilePath);
    }
#elif defined(Q_OS_WIN)
    QSettings autostartSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    
    if (enable)
    {
        autostartSettings.setValue("UnfuckMyTimeZoneMath", QCoreApplication::applicationFilePath().replace('/', '\\'));
    }
    else
    {
        autostartSettings.remove("UnfuckMyTimeZoneMath");
    }
#elif defined(Q_OS_MACOS)
    QString launchAgentsPath = QDir::homePath() + "/Library/LaunchAgents";
    QDir launchAgentsDir(launchAgentsPath);
    
    if (!launchAgentsDir.exists())
    {
        launchAgentsDir.mkpath(".");
    }
    
    QString plistPath = launchAgentsPath + "/com.ows.unfuckmytimezonemath.plist";
    
    if (enable)
    {
        QFile plistFile(plistPath);
        if (plistFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&plistFile);
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            out << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
            out << "<plist version=\"1.0\">\n";
            out << "<dict>\n";
            out << "    <key>Label</key>\n";
            out << "    <string>com.ows.unfuckmytimezonemath</string>\n";
            out << "    <key>ProgramArguments</key>\n";
            out << "    <array>\n";
            out << "        <string>" << QCoreApplication::applicationFilePath() << "</string>\n";
            out << "    </array>\n";
            out << "    <key>RunAtLoad</key>\n";
            out << "    <true/>\n";
            out << "</dict>\n";
            out << "</plist>\n";
            plistFile.close();
        }
    }
    else
    {
        QFile::remove(plistPath);
    }
#endif
}
