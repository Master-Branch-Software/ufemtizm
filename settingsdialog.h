#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    bool showInTaskBar() const;
    bool minimizeToTray() const;
    bool closeToTray() const;
    bool startMinimized() const;
    bool runAtLogin() const;

    void setShowInTaskBar(bool show);
    void setMinimizeToTray(bool minimize);
    void setCloseToTray(bool close);
    void setStartMinimized(bool start);
    void setRunAtLogin(bool run);

private slots:
    void onAccepted();

private:
    void loadSettings();
    void saveSettings();

    QCheckBox *showInTaskBarCheckBox;
    QCheckBox *minimizeToTrayCheckBox;
    QCheckBox *closeToTrayCheckBox;
    QCheckBox *startMinimizedCheckBox;
    QCheckBox *runAtLoginCheckBox;
    QDialogButtonBox *buttonBox;
    
    void setupAutostart(bool enable);
};

#endif
