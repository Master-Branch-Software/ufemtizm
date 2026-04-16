#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // Returns the custom display name, or the real app name if blank.
    static QString effectiveDisplayName();

    bool showInTaskBar() const;
    bool minimizeToTray() const;
    bool closeToTray() const;
    bool startMinimized() const;
    bool runAtLogin() const;
    bool skyColorEnabled() const;

    void setShowInTaskBar(bool show);
    void setMinimizeToTray(bool minimize);
    void setCloseToTray(bool close);
    void setStartMinimized(bool start);
    void setRunAtLogin(bool run);
    void setSkyColorEnabled(bool enabled);

private slots:
    void onAccepted();

private:
    void loadSettings();
    void saveSettings();

    QLineEdit *displayNameLineEdit;
    QCheckBox *showInTaskBarCheckBox;
    QCheckBox *minimizeToTrayCheckBox;
    QCheckBox *closeToTrayCheckBox;
    QCheckBox *startMinimizedCheckBox;
    QCheckBox *runAtLoginCheckBox;
    QCheckBox *skyColorCheckBox;
    QDialogButtonBox *buttonBox;
    
    void setupAutostart(bool enable);
};

#endif // SETTINGSDIALOG_HPP
