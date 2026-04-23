#include "mainwindow.hpp"
#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QLocalSocket>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>

static void configureSnapPersistentSettings(){
    // When running as a snap, $SNAP_USER_DATA (mapped to $HOME at runtime) is
    // revision-scoped and not reliably copied forward on sideloaded installs.
    // Redirect QSettings to $SNAP_USER_COMMON so settings persist across every
    // revision of the snap.
    QByteArray snapUserCommon = qgetenv("SNAP_USER_COMMON");

    if (snapUserCommon.isEmpty()){
        return;
    }

    QString commonDir = QString::fromLocal8Bit(snapUserCommon);
    QDir().mkpath(commonDir);

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, commonDir);

    // Migrate a previous revision's config file if the new location is empty.
    QString newConfigPath = commonDir + "/Ufemtizm/Ufemtizm.ini";

    if (QFile::exists(newConfigPath)){
        return;
    }

    QByteArray snapUserData = qgetenv("SNAP_USER_DATA");
    QString oldConfigPath = QString::fromLocal8Bit(snapUserData) +
                            "/.config/Ufemtizm/Ufemtizm.conf";

    if (snapUserData.isEmpty() || !QFile::exists(oldConfigPath)){
        return;
    }

    QDir().mkpath(QFileInfo(newConfigPath).absolutePath());
    QFile::copy(oldConfigPath, newConfigPath);
}

int main(int argc, char *argv[]){
    // Fix cursor size scaling issue on Linux - only set if not already defined
    if (qEnvironmentVariableIsEmpty("XCURSOR_SIZE")){
        qputenv("XCURSOR_SIZE", "24");
    }

    configureSnapPersistentSettings();

    QApplication app(argc, argv);

    // Identify the application to the desktop environment so the dock/taskbar
    // associates running windows with the installed .desktop file and icon.
    app.setApplicationName("Ufemtizm");
    app.setApplicationDisplayName("Ufemtizm");
    app.setOrganizationName("Ufemtizm");
#ifdef Q_OS_LINUX
    app.setDesktopFileName("ufemtizm");
#endif
    app.setWindowIcon(QIcon(":/icons/icon.png"));

    // Hide from dock/taskbar on all platforms - app is only accessible via system tray
    app.setQuitOnLastWindowClosed(false);

    QString serverName = "Ufemtizm_SingleInstance";
    QLocalSocket socket;
    socket.connectToServer(serverName);

    if (socket.waitForConnected(500)){
        return 0;
    }
    
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QString appStyleSheet = 
        "QWidget {"
        "    font-family: -apple-system, 'Segoe UI', 'Helvetica Neue', Arial, sans-serif;"
        "}"
        "QScrollBar:vertical {"
        "    border: none;"
        "    background: #eef4fa;"
        "    width: 10px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #a9b3bb;"
        "    border-radius: 5px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #727c83;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "    border: none;"
        "    background: #eef4fa;"
        "    height: 10px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: #a9b3bb;"
        "    border-radius: 5px;"
        "    min-width: 20px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background: #727c83;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    width: 0px;"
        "}"
        "QToolTip {"
        "    background-color: #2a343a;"
        "    color: #fbf7ff;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 6px 10px;"
        "    font-size: 12px;"
        "}";
    
    app.setStyleSheet(appStyleSheet);
    
    MainWindow window;
    
    QSettings settings("Ufemtizm", "Ufemtizm");
    bool startMinimized = settings.value("systemTray/startMinimized", false).toBool();
    
    // Show window unless user has explicitly configured to start minimized
    if (!startMinimized){
#ifdef Q_OS_LINUX
        // On Linux/KDE, the window must be shown first, then events processed,
        // then repainted to prevent the black window on first display.
        window.show();
        window.activateWindow();
        window.raise();
        app.processEvents();
        window.repaint();
        app.processEvents();
#else
        // Process pending events to ensure window is properly initialized before showing
        app.processEvents();
        window.show();
        // Force a paint event to avoid black/corrupt window on first display
        window.repaint();
#endif
    }
    
    return app.exec();
}
