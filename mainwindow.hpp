#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QSystemTrayIcon>

class TimeZoneWidget;
class QMenu;
class QToolBar;
class QSystemTrayIcon;
class SettingsDialog;
class CopyDialog;
class QLocalServer;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void openRecentFile();
    void addTimeZoneWidget();
    void removeTimeZoneWidget(TimeZoneWidget *widget);
    void onTimeChanged(qint64 baseTimestamp);
    void onWidgetModified();
    void showAboutDialog();
    void onWidgetDropped(TimeZoneWidget *target, TimeZoneWidget *source);
    void setToCurrentTime();
    void toggleToolBarVisibility();
    void toggleToolBarTextVisibility();
    void showSettings();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void toggleWindowVisibility();
    void quitApplication();
    void openRecentFileFromTray();
    void updateTrayMenu();
    void handleNewConnection();
    void copyToClipboard();
    void copyToClipboardDirect();
    void initializeSystemTray();
    void toggleSkyColor();

private:
    void setupMenuBar();
    void setupToolBar();
    void updateAllWidgets(qint64 baseTimestamp, TimeZoneWidget *source);
    void updateWindowTitle();
    void updateRecentFilesMenu();
    void adjustWindowSize();
    void saveWindowGeometry();
    void restoreWindowGeometry();
    bool maybeSave();
    bool saveToFile(const QString &filename);
    bool loadFromFile(const QString &filename);
    void setCurrentFile(const QString &filename);
    void addRecentFile(const QString &filename);
    QStringList getRecentFiles() const;
    void performCopy(const QVector<int> &indices);

    QWidget *centralWidget;
    QVector<TimeZoneWidget*> timeZoneWidgets;
    QMenu *recentFilesMenu;
    QToolBar *toolBar;
    QAction *toggleToolBarAction;
    QAction *toggleToolBarTextAction;
    QAction *toolBarTextToggleAction;
    QAction *toolBarHideAction;
    QList<QAction*> mainToolBarActions;
    QString currentFilename;
    bool isDirty;
    
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QMenu *trayRecentFilesMenu;
    SettingsDialog *settingsDialog;
    QLocalServer *localServer;
    bool forceQuit;
    bool initialSizeSet;
    QTimer *trayInitTimer;
    int trayInitAttempts;
};

#endif // MAINWINDOW_HPP
