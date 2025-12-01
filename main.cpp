#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QLocalSocket>
#include <QMessageBox>
#include <QSettings>

int main(int argc, char *argv[])
{
    // Fix cursor size scaling issue on Linux - only set if not already defined
    if (qEnvironmentVariableIsEmpty("XCURSOR_SIZE"))
    {
        qputenv("XCURSOR_SIZE", "24");
    }
    
    QApplication app(argc, argv);
    
    // Hide from dock/taskbar on all platforms - app is only accessible via system tray
    app.setQuitOnLastWindowClosed(false);
    
    QString serverName = "UnfuckMyTimeZoneMath_SingleInstance";
    QLocalSocket socket;
    socket.connectToServer(serverName);
    
    if (socket.waitForConnected(500))
    {
        return 0;
    }
    
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QString appStyleSheet = 
        "QWidget {"
        "    font-family: -apple-system, 'Segoe UI', 'Helvetica Neue', Arial, sans-serif;"
        "}"
        "QScrollBar:vertical {"
        "    border: none;"
        "    background: #f5f5f5;"
        "    width: 10px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #bdbdbd;"
        "    border-radius: 5px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #9e9e9e;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "    border: none;"
        "    background: #f5f5f5;"
        "    height: 10px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: #bdbdbd;"
        "    border-radius: 5px;"
        "    min-width: 20px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background: #9e9e9e;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    width: 0px;"
        "}"
        "QToolTip {"
        "    background-color: #424242;"
        "    color: #ffffff;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 10px;"
        "    font-size: 12px;"
        "}"
        "QFileDialog {"
        "    background-color: #ffffff;"
        "}"
        "QFileDialog QPushButton {"
        "    background-color: #f5f5f5;"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    padding: 6px 16px;"
        "}"
        "QFileDialog QPushButton:hover {"
        "    background-color: #e3f2fd;"
        "    border: 1px solid #2196f3;"
        "}"
        "QFileDialog QPushButton:default {"
        "    background-color: #2196f3;"
        "    color: #ffffff;"
        "    border: 1px solid #1976d2;"
        "}";
    
    app.setStyleSheet(appStyleSheet);
    
    MainWindow window;
    window.setWindowIcon(QIcon(":/icons/icon.png"));
    
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    bool startMinimized = settings.value("systemTray/startMinimized", false).toBool();
    
    // Show window unless user has explicitly configured to start minimized
    if (!startMinimized)
    {
#ifdef Q_OS_LINUX
        // On Linux, ensure window is fully initialized before showing
        // This prevents the black window issue on KDE/Plasma
        window.show();
        window.activateWindow();
        window.raise();
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
