#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QStringList>

class TimeZoneWidget;
class QMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

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

private:
    void setupMenuBar();
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

    QWidget *centralWidget;
    QVector<TimeZoneWidget*> timeZoneWidgets;
    QMenu *recentFilesMenu;
    QString currentFilename;
    bool isDirty;
};

#endif // MAINWINDOW_H
