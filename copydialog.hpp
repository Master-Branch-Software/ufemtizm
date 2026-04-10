#ifndef COPYDIALOG_HPP
#define COPYDIALOG_HPP

#include <QDialog>
#include <QVector>
#include <QStringList>

class QCheckBox;
class QVBoxLayout;

class CopyDialog : public QDialog
{
    Q_OBJECT

private:
    QVector<QCheckBox*> mTileCheckBoxes;
    QCheckBox *mDontShowAgainCheckBox;

    void setupUI(const QStringList &tileNames, bool showDontShowAgain);

public:
    explicit CopyDialog(const QStringList &tileNames, bool showDontShowAgain = true,
                        QWidget *parent = nullptr);

    QVector<int> selectedIndices() const;
    bool dontShowAgain() const;
};

#endif // COPYDIALOG_HPP
