#include "copydialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

CopyDialog::CopyDialog(const QStringList &tileNames, bool showDontShowAgain,
                       QWidget *parent) :
    QDialog(parent),
    mDontShowAgainCheckBox(nullptr)
{
    setWindowTitle("Copy to Clipboard");
    setupUI(tileNames, showDontShowAgain);
}

void CopyDialog::setupUI(const QStringList &tileNames, bool showDontShowAgain){
    setStyleSheet(
        "QDialog {"
        "    background-color: #f6fafe;"
        "}"
        "QLabel {"
        "    color: #2a343a;"
        "}"
        "QCheckBox {"
        "    color: #2a343a;"
        "    spacing: 8px;"
        "    padding: 6px 4px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 18px;"
        "    height: 18px;"
        "    border-radius: 4px;"
        "    border: 2px solid #a9b3bb;"
        "    background-color: #ffffff;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: #4e45e4;"
        "    border: 2px solid #4e45e4;"
        "}"
        "QCheckBox::indicator:hover {"
        "    border: 2px solid #4e45e4;"
        "}"
        "QPushButton {"
        "    background-color: #eef4fa;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 20px;"
        "    color: #2a343a;"
        "    font-weight: 500;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e7eff5;"
        "}"
        "QPushButton#copyButton {"
        "    background-color: #4e45e4;"
        "    color: #fbf7ff;"
        "}"
        "QPushButton#copyButton:hover {"
        "    background-color: #4135d8;"
        "}"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(12);

    QLabel *headerLabel = new QLabel("Select timezones to copy:");

    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(13);
    headerFont.setWeight(QFont::Medium);
    headerLabel->setFont(headerFont);

    mainLayout->addWidget(headerLabel);

    QFrame *listFrame = new QFrame();
    listFrame->setStyleSheet(
        "QFrame {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 12px;"
        "    padding: 8px;"
        "}"
    );

    QVBoxLayout *listLayout = new QVBoxLayout(listFrame);
    listLayout->setContentsMargins(12, 8, 12, 8);
    listLayout->setSpacing(4);

    for (int i = 0; i < tileNames.size(); i++){
        QCheckBox *checkBox = new QCheckBox(tileNames[i]);
        checkBox->setChecked(true);

        QFont checkFont = checkBox->font();
        checkFont.setPointSize(12);
        checkBox->setFont(checkFont);

        mTileCheckBoxes.append(checkBox);
        listLayout->addWidget(checkBox);
    }

    mainLayout->addWidget(listFrame);

    if (showDontShowAgain){
        mainLayout->addSpacing(4);

        mDontShowAgainCheckBox = new QCheckBox("Don't show this again");

        QFont dontShowFont = mDontShowAgainCheckBox->font();
        dontShowFont.setPointSize(11);
        mDontShowAgainCheckBox->setFont(dontShowFont);
        mDontShowAgainCheckBox->setStyleSheet(
            "QCheckBox {"
            "    color: #727c83;"
            "    padding: 4px;"
            "}"
        );

        mainLayout->addWidget(mDontShowAgainCheckBox);
    }

    mainLayout->addSpacing(8);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton *copyButton = new QPushButton("Copy");
    copyButton->setObjectName("copyButton");
    copyButton->setDefault(true);
    connect(copyButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(copyButton);

    mainLayout->addLayout(buttonLayout);

    setMinimumWidth(300);
    adjustSize();
}

QVector<int> CopyDialog::selectedIndices() const{
    QVector<int> indices;

    for (int i = 0; i < mTileCheckBoxes.size(); i++){
        if (mTileCheckBoxes[i]->isChecked()){
            indices.append(i);
        }
    }

    return indices;
}

bool CopyDialog::dontShowAgain() const{
    if (mDontShowAgainCheckBox){
        return mDontShowAgainCheckBox->isChecked();
    }

    return false;
}
