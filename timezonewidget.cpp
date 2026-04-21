#include "timezonewidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QMenu>
#include <QFocusEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsColorizeEffect>
#include <QSettings>
#include <QtMath>
#include <algorithm>

EditableLabel::EditableLabel(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    mLabel = new QLabel(text, this);
    mLabel->setAlignment(Qt::AlignCenter);
    mLabel->setStyleSheet(
        "QLabel {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 12px;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    color: #2a343a;"
        "}"
        "QLabel:hover {"
        "    background-color: #eef4fa;"
        "}"
    );
    mLabel->setCursor(Qt::IBeamCursor);
    
    mLineEdit = new QLineEdit(text, this);
    mLineEdit->setFocusPolicy(Qt::StrongFocus);
    mLineEdit->setStyleSheet(
        "QLineEdit {"
        "    background-color: #eef4fa;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 12px;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    color: #2a343a;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid #4e45e4;"
        "    background-color: #ffffff;"
        "}"
    );
    mLineEdit->hide();
    mLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    
    layout->addWidget(mLabel);
    layout->addWidget(mLineEdit);
    
    connect(mLineEdit, &QLineEdit::editingFinished, this, &EditableLabel::finishEditing);
    connect(mLineEdit, &QLineEdit::textChanged, this, &EditableLabel::textChanged);
}

QString EditableLabel::text() const{
    return mLabel->text();
}

void EditableLabel::setText(const QString &text){
    mLabel->setText(text);
    mLineEdit->setText(text);
}

void EditableLabel::setTextColor(const QColor &color){
    QString styleSheet = QString(
        "QLabel {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 12px;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    color: %1;"
        "}"
        "QLabel:hover {"
        "    background-color: rgba(238, 244, 250, 0.5);"
        "}"
    ).arg(color.name());
    mLabel->setStyleSheet(styleSheet);
}

void EditableLabel::selectAll(){
    mLineEdit->setFocus();
    mLineEdit->selectAll();
}

bool EditableLabel::isEditing() const{
    return mLineEdit->isVisible();
}

void EditableLabel::mouseDoubleClickEvent(QMouseEvent *event){
    Q_UNUSED(event);
    mLabel->hide();
    mLineEdit->setText(mLabel->text());
    mLineEdit->show();
    mLineEdit->setFocus();
    mLineEdit->selectAll();
}

void EditableLabel::finishEditing(){
    if (!mLineEdit->isVisible()){
        return;
    }
    mLabel->setText(mLineEdit->text());
    mLineEdit->hide();
    mLabel->show();
}

TimeZoneWidget::TimeZoneWidget(QWidget *parent)
    : QWidget(parent),
      baseTimestamp(QDateTime::currentSecsSinceEpoch()),
      currentTimeZone(QTimeZone::systemTimeZone()),
      is24HourFormat(true),
      updatingInternally(false),
      skyColorEnabled(true),
      dropIndicator(nullptr),
      frameShadow(nullptr)
{
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    skyColorEnabled = settings.value("appearance/skyColor", true).toBool();
    
    setupUI();
    populateTimeZones();
    updateDisplay();
    updateSliderLabels();
}

void TimeZoneWidget::setupUI(){
    setMinimumWidth(240);
    setMaximumWidth(280);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setAcceptDrops(true);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    dropIndicator = new QFrame(this);
    dropIndicator->setFixedWidth(4);
    dropIndicator->setStyleSheet(
        "QFrame {"
        "    background-color: #4e45e4;"
        "    border-radius: 2px;"
        "}"
    );
    dropIndicator->hide();
    
    frame = new QFrame();
    frame->setObjectName("timeZoneCard");
    frame->setStyleSheet(
        "QFrame#timeZoneCard {"
        "    background-color: #ffffff;"
        "    border: none;"
        "    border-radius: 24px;"
        "    padding: 0px;"
        "}"
    );
    frame->setGraphicsEffect(nullptr);
    frameShadow = new QGraphicsDropShadowEffect();
    frameShadow->setBlurRadius(32);
    frameShadow->setXOffset(0);
    frameShadow->setYOffset(4);
    frameShadow->setColor(QColor(42, 52, 58, 20));
    frame->setGraphicsEffect(frameShadow);
    
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(12, 12, 12, 12);
    frameLayout->setSpacing(8);
    
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(4);
    
    format24Button = new QToolButton();
    format24Button->setText("24");
    format24Button->setFixedSize(28, 28);
    format24Button->setStyleSheet(
        "QToolButton {"
        "    background-color: #eef4fa;"
        "    color: #566167;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 11px;"
        "    font-weight: 500;"
        "}"
        "QToolButton:hover {"
        "    background-color: #e7eff5;"
        "}"
        "QToolButton:pressed {"
        "    background-color: #d9e4ec;"
        "}"
    );
    format24Button->setToolTip("Toggle time format");
    format24Button->setCursor(Qt::PointingHandCursor);
    connect(format24Button, &QToolButton::clicked, this, [this](){
        is24HourFormat = !is24HourFormat;
        format24Button->setText(is24HourFormat ? "24" : "12");
        updateDisplay();
        updateSliderLabels();
        emit widgetModified();
    });
    
    timeZoneCombo = new QComboBox();
    timeZoneCombo->hide();
    connect(timeZoneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &TimeZoneWidget::onTimeZoneChanged);
    
    globeButton = new QToolButton();
    globeButton->setText("🌍");
    globeButton->setFixedSize(28, 28);
    globeButton->setStyleSheet(
        "QToolButton {"
        "    background-color: #eef4fa;"
        "    color: #566167;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 14px;"
        "}"
        "QToolButton:hover {"
        "    background-color: #e7eff5;"
        "}"
        "QToolButton:pressed {"
        "    background-color: #d9e4ec;"
        "}"
    );
    globeButton->setToolTip("Select timezone");
    globeButton->setCursor(Qt::PointingHandCursor);
    connect(globeButton, &QToolButton::clicked, this, [this](){
        QMenu *menu = new QMenu(this);
        for (int i = 0; i < timeZoneCombo->count(); i++){
            QAction *action = menu->addAction(timeZoneCombo->itemText(i));
            action->setData(i);
            if (i == timeZoneCombo->currentIndex()){
                QFont font = action->font();
                font.setBold(true);
                action->setFont(font);
            }
        }
        connect(menu, &QMenu::triggered, this, [this](QAction *action){
            int index = action->data().toInt();
            timeZoneCombo->setCurrentIndex(index);
        });
        menu->exec(globeButton->mapToGlobal(globeButton->rect().bottomLeft()));
    });
    
    toolbarLayout->addWidget(format24Button);
    toolbarLayout->addWidget(globeButton);
    toolbarLayout->addStretch();
    
    removeButton = new QPushButton("×");
    removeButton->setFixedSize(24, 24);
    removeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #a9b3bb;"
        "    border: none;"
        "    border-radius: 12px;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #eef4fa;"
        "    color: #566167;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #e7eff5;"
        "}"
    );
    removeButton->setToolTip("Remove this timezone widget");
    removeButton->setCursor(Qt::PointingHandCursor);
    connect(removeButton, &QPushButton::clicked, this, [this](){
        emit removeRequested(this);
    });
    
    toolbarLayout->addWidget(removeButton);
    frameLayout->addLayout(toolbarLayout);
    
    nameEdit = new EditableLabel("My Wonderful Self");
    nameEdit->setToolTip("Double-click to edit name");
    connect(nameEdit, &EditableLabel::textChanged, this, &TimeZoneWidget::widgetModified);
    frameLayout->addWidget(nameEdit);
    
    dateTimeLabel = new QLabel("Wed, 12:00a");
    dateTimeLabel->setAlignment(Qt::AlignCenter);
    QFont dtFont = dateTimeLabel->font();
    dtFont.setPointSize(18);
    dtFont.setBold(true);
    dateTimeLabel->setFont(dtFont);
    dateTimeLabel->setStyleSheet(
        "QLabel {"
        "    color: #2a343a;"
        "    padding: 2px 4px 0px 4px;"
        "    margin-bottom: 0px;"
        "}"
    );
    frameLayout->addWidget(dateTimeLabel);
    
    timeZoneLabel = new QLabel("UTC");
    timeZoneLabel->setAlignment(Qt::AlignCenter);
    QFont tzFont = timeZoneLabel->font();
    tzFont.setPointSize(10);
    timeZoneLabel->setFont(tzFont);
    timeZoneLabel->setStyleSheet(
        "QLabel {"
        "    color: #727c83;"
        "    padding: 0px 2px 0px 2px;"
        "    margin-top: 0px;"
        "    margin-bottom: 8px;"
        "}"
    );
    frameLayout->addWidget(timeZoneLabel);
    
    dayOffsetLabel = new QLabel("+0 Days");
    dayOffsetLabel->setAlignment(Qt::AlignCenter);
    QFont offsetFont = dayOffsetLabel->font();
    offsetFont.setPointSize(11);
    offsetFont.setWeight(QFont::Medium);
    dayOffsetLabel->setFont(offsetFont);
    dayOffsetLabel->setStyleSheet(
        "QLabel {"
        "    color: #566167;"
        "    padding: 4px;"
        "    margin-top: 4px;"
        "}"
    );
    frameLayout->addWidget(dayOffsetLabel);
    
    timeSlider = new QSlider(Qt::Vertical);
    timeSlider->setMinimum(0);
    timeSlider->setMaximum(96);
    timeSlider->setValue(48);
    timeSlider->setTickPosition(QSlider::TicksRight);
    timeSlider->setTickInterval(4);
    timeSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    timeSlider->setInvertedAppearance(true);
    timeSlider->setInvertedControls(true);
    timeSlider->setStyleSheet(
        "QSlider::groove:vertical {"
        "    background: #d9e4ec;"
        "    width: 4px;"
        "    border-radius: 2px;"
        "}"
        "QSlider::handle:vertical {"
        "    background: #4e45e4;"
        "    border: 2px solid #4135d8;"
        "    width: 16px;"
        "    height: 16px;"
        "    margin: -8px -6px;"
        "    border-radius: 8px;"
        "}"
        "QSlider::handle:vertical:hover {"
        "    background: #6760fd;"
        "    border: 2px solid #4e45e4;"
        "}"
        "QSlider::add-page:vertical {"
        "    background: transparent;"
        "}"
        "QSlider::sub-page:vertical {"
        "    background: transparent;"
        "}"
    );
    timeSlider->setToolTip("Drag to adjust time");
    connect(timeSlider, &QSlider::valueChanged, this, &TimeZoneWidget::onSliderValueChanged);
    
    sliderContainer = new QWidget();
    QHBoxLayout *sliderContainerLayout = new QHBoxLayout(sliderContainer);
    sliderContainerLayout->setContentsMargins(0, 0, 0, 0);
    sliderContainerLayout->setSpacing(3);
    
    QWidget *labelsWidget = new QWidget();
    labelsWidget->setFixedWidth(32);
    labelsWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QVBoxLayout *labelsLayout = new QVBoxLayout(labelsWidget);
    labelsLayout->setContentsMargins(0, 0, 0, 0);
    labelsLayout->setSpacing(0);
    
    for (int i = 0; i <= 24; i++){
        QLabel *hourLabel = new QLabel();
        hourLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        hourLabel->setFixedWidth(32);
        hourLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        QFont labelFont = hourLabel->font();
        labelFont.setPointSize(9);
        labelFont.setWeight(QFont::Medium);
        hourLabel->setFont(labelFont);
        hourLabel->setStyleSheet(
            "QLabel {"
            "    color: #566167;"
            "    text-align: right;"
            "}"
        );
        hourLabels.append(hourLabel);
        labelsLayout->addWidget(hourLabel, 1);
        
        if (i < 24){
            labelsLayout->addSpacing(0);
        }
    }
    
    sliderContainerLayout->addWidget(labelsWidget);
    sliderContainerLayout->addWidget(timeSlider);
    
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    sliderLayout->addStretch();
    sliderLayout->addWidget(sliderContainer);
    sliderLayout->addStretch();
    frameLayout->addLayout(sliderLayout, 1);
    
    mainLayout->addWidget(frame);
}

void TimeZoneWidget::populateTimeZones(){
    struct TimeZoneEntry
    {
        QString displayName;
        QByteArray tzId;
        int offsetSeconds;
        bool isUS;
    };
    
    QVector<TimeZoneEntry> majorCities = {
        {"Honolulu (UTC-10)", "Pacific/Honolulu", -10 * 3600, true},
        {"Anchorage (UTC-9)", "America/Anchorage", -9 * 3600, true},
        {"Los Angeles (UTC-8)", "America/Los_Angeles", -8 * 3600, true},
        {"Phoenix (UTC-7)", "America/Phoenix", -7 * 3600, true},
        {"Denver (UTC-7)", "America/Denver", -7 * 3600, true},
        {"Chicago (UTC-6)", "America/Chicago", -6 * 3600, true},
        {"New York (UTC-5)", "America/New_York", -5 * 3600, true},
        {"Baker Island (UTC-12)", "Etc/GMT+12", -12 * 3600, false},
        {"Pago Pago (UTC-11)", "Pacific/Pago_Pago", -11 * 3600, false},
        {"Caracas (UTC-4)", "America/Caracas", -4 * 3600, false},
        {"Buenos Aires (UTC-3)", "America/Argentina/Buenos_Aires", -3 * 3600, false},
        {"Sao Paulo (UTC-3)", "America/Sao_Paulo", -3 * 3600, false},
        {"Nuuk (UTC-2)", "America/Nuuk", -2 * 3600, false},
        {"Azores (UTC-1)", "Atlantic/Azores", -1 * 3600, false},
        {"London (UTC+0)", "Europe/London", 0, false},
        {"Reykjavik (UTC+0)", "Atlantic/Reykjavik", 0, false},
        {"Paris (UTC+1)", "Europe/Paris", 1 * 3600, false},
        {"Berlin (UTC+1)", "Europe/Berlin", 1 * 3600, false},
        {"Cairo (UTC+2)", "Africa/Cairo", 2 * 3600, false},
        {"Athens (UTC+2)", "Europe/Athens", 2 * 3600, false},
        {"Moscow (UTC+3)", "Europe/Moscow", 3 * 3600, false},
        {"Istanbul (UTC+3)", "Europe/Istanbul", 3 * 3600, false},
        {"Dubai (UTC+4)", "Asia/Dubai", 4 * 3600, false},
        {"Karachi (UTC+5)", "Asia/Karachi", 5 * 3600, false},
        {"Mumbai (UTC+5:30)", "Asia/Kolkata", 5 * 3600 + 1800, false},
        {"Dhaka (UTC+6)", "Asia/Dhaka", 6 * 3600, false},
        {"Bangkok (UTC+7)", "Asia/Bangkok", 7 * 3600, false},
        {"Hong Kong (UTC+8)", "Asia/Hong_Kong", 8 * 3600, false},
        {"Singapore (UTC+8)", "Asia/Singapore", 8 * 3600, false},
        {"Tokyo (UTC+9)", "Asia/Tokyo", 9 * 3600, false},
        {"Seoul (UTC+9)", "Asia/Seoul", 9 * 3600, false},
        {"Sydney (UTC+10)", "Australia/Sydney", 10 * 3600, false},
        {"Brisbane (UTC+10)", "Australia/Brisbane", 10 * 3600, false},
        {"Noumea (UTC+11)", "Pacific/Noumea", 11 * 3600, false},
        {"Auckland (UTC+12)", "Pacific/Auckland", 12 * 3600, false},
        {"Fiji (UTC+12)", "Pacific/Fiji", 12 * 3600, false},
        {"Tongatapu (UTC+13)", "Pacific/Tongatapu", 13 * 3600, false},
    };
    
    QVector<TimeZoneEntry> usCities;
    QVector<TimeZoneEntry> otherCities;
    
    for (const TimeZoneEntry &entry : majorCities){
        if (entry.isUS){
            usCities.append(entry);
        }
        else{
            otherCities.append(entry);
        }
    }
    
    std::sort(usCities.begin(), usCities.end(), [](const TimeZoneEntry &a, const TimeZoneEntry &b)
    {
        return a.offsetSeconds < b.offsetSeconds;
    });
    
    std::sort(otherCities.begin(), otherCities.end(), [](const TimeZoneEntry &a, const TimeZoneEntry &b)
    {
        return a.offsetSeconds < b.offsetSeconds;
    });
    
    for (const TimeZoneEntry &entry : usCities){
        timeZoneCombo->addItem(entry.displayName, entry.tzId);
    }
    
    timeZoneCombo->insertSeparator(timeZoneCombo->count());
    
    for (const TimeZoneEntry &entry : otherCities){
        timeZoneCombo->addItem(entry.displayName, entry.tzId);
    }
    
    QByteArray systemTzId = QTimeZone::systemTimeZoneId();
    int index = timeZoneCombo->findData(systemTzId);
    
    if (index != -1){
        timeZoneCombo->setCurrentIndex(index);
    }
}

void TimeZoneWidget::setBaseTimestamp(qint64 timestamp)
{
    if (baseTimestamp == timestamp){
        return;
    }
    
    updatingInternally = true;
    baseTimestamp = timestamp;
    
    int sliderValue = timestampToSliderValue(timestamp);
    timeSlider->setValue(sliderValue);
    
    updateDisplay();
    updatingInternally = false;
}

qint64 TimeZoneWidget::getBaseTimestamp() const
{
    return baseTimestamp;
}

void TimeZoneWidget::updateDisplay(){
    QDateTime baseDateTime = QDateTime::fromSecsSinceEpoch(baseTimestamp, QTimeZone::utc());
    QDateTime localDateTime = baseDateTime.toTimeZone(currentTimeZone);
    
    QString timeFormat = is24HourFormat ? "HH:mm" : "h:mma";
    
    dateTimeLabel->setText(localDateTime.toString(timeFormat));
    updateTimeZoneLabel();
    updateSkyColor();
    
    QDateTime referenceDateTime = QDateTime::fromSecsSinceEpoch(baseTimestamp, QTimeZone::utc());
    QDateTime referenceLocal = referenceDateTime.toTimeZone(QTimeZone::systemTimeZone());
    QDateTime widgetLocal = referenceDateTime.toTimeZone(currentTimeZone);
    
    int refDay = referenceLocal.date().toJulianDay();
    int widgetDay = widgetLocal.date().toJulianDay();
    int dayOffset = widgetDay - refDay;
    
    if (dayOffset == 0){
        dayOffsetLabel->setText("Same Day");
    }
    else if (dayOffset > 0)
    {
        dayOffsetLabel->setText(QString("+%1 Day%2").arg(dayOffset).arg(dayOffset > 1 ? "s" : ""));
    }
    else{
        dayOffsetLabel->setText(QString("%1 Day%2").arg(dayOffset).arg(dayOffset < -1 ? "s" : ""));
    }
}

void TimeZoneWidget::onSliderValueChanged(int value)
{
    if (updatingInternally){
        return;
    }
    
    baseTimestamp = sliderValueToTimestamp(value);
    updateDisplay();
    
    emit timeChanged(baseTimestamp);
}

void TimeZoneWidget::onTimeZoneChanged(int index)
{
    if (index < 0 || updatingInternally){
        return;
    }
    
    QByteArray tzId = timeZoneCombo->itemData(index).toByteArray();
    currentTimeZone = QTimeZone(tzId);
    
    int sliderValue = timestampToSliderValue(baseTimestamp);
    updatingInternally = true;
    timeSlider->setValue(sliderValue);
    updatingInternally = false;
    
    updateDisplay();
    emit widgetModified();
}

void TimeZoneWidget::onFormatChanged(int state)
{
    is24HourFormat = (state == Qt::Checked);
    updateDisplay();
    updateSliderLabels();
    emit widgetModified();
}

void TimeZoneWidget::updateTimeZoneLabel(){
    QString tzName = QString::fromUtf8(currentTimeZone.id());
    tzName = tzName.section('/', -1);
    tzName.replace('_', ' ');
    timeZoneLabel->setText(tzName);
}

void TimeZoneWidget::updateSkyColor(){
    QDateTime baseDateTime = QDateTime::fromSecsSinceEpoch(baseTimestamp, QTimeZone::utc());
    QDateTime localDateTime = baseDateTime.toTimeZone(currentTimeZone);
    
    int hour = localDateTime.time().hour();
    int minute = localDateTime.time().minute();
    double timeInHours = hour + (minute / 60.0);
    
    bool isNightTime = (timeInHours >= 20.0 || timeInHours < 7.0);
    
    if (!skyColorEnabled){
        frame->setStyleSheet(
            "QFrame#timeZoneCard {"
            "    background-color: #ffffff;"
            "    border: none;"
            "    border-radius: 24px;"
            "    padding: 0px;"
            "}"
        );
        dateTimeLabel->setStyleSheet(
            "QLabel {"
            "    color: #2a343a;"
            "    padding: 2px 4px 0px 4px;"
            "    margin-bottom: 0px;"
            "}"
        );
        timeZoneLabel->setStyleSheet(
            "QLabel {"
            "    color: #727c83;"
            "    padding: 0px 2px 0px 2px;"
            "    margin-top: 0px;"
            "    margin-bottom: 8px;"
            "}"
        );
        dayOffsetLabel->setStyleSheet(
            "QLabel {"
            "    color: #566167;"
            "    padding: 4px;"
            "    margin-top: 4px;"
            "}"
        );
        nameEdit->setTextColor(QColor(0x2a, 0x34, 0x3a));
        
        format24Button->setStyleSheet(
            "QToolButton {"
            "    background-color: #eef4fa;"
            "    color: #566167;"
            "    border: none;"
            "    border-radius: 8px;"
            "    font-size: 11px;"
            "    font-weight: 500;"
            "}"
            "QToolButton:hover {"
            "    background-color: #e7eff5;"
            "}"
            "QToolButton:pressed {"
            "    background-color: #d9e4ec;"
            "}"
        );
        
        globeButton->setStyleSheet(
            "QToolButton {"
            "    background-color: #eef4fa;"
            "    color: #566167;"
            "    border: none;"
            "    border-radius: 8px;"
            "    font-size: 14px;"
            "}"
            "QToolButton:hover {"
            "    background-color: #e7eff5;"
            "}"
            "QToolButton:pressed {"
            "    background-color: #d9e4ec;"
            "}"
        );
        
        removeButton->setStyleSheet(
            "QPushButton {"
            "    background-color: transparent;"
            "    color: #a9b3bb;"
            "    border: none;"
            "    border-radius: 12px;"
            "    font-size: 20px;"
            "    font-weight: bold;"
            "    padding: 0px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #eef4fa;"
            "    color: #566167;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #e7eff5;"
            "}"
        );
        
        return;
    }
    
    QColor skyColor = calculateSkyColor(localDateTime);
    
    // Calculate darker button color (20% darker than background)
    QColor buttonColor = skyColor.darker(120);
    
    frame->setStyleSheet(
        QString("QFrame#timeZoneCard {"
        "    background-color: %1;"
        "    border: none;"
        "    border-radius: 24px;"
        "    padding: 0px;"
        "}").arg(skyColor.name())
    );
    
    // Determine text color based on sky color brightness
    int brightness = (skyColor.red() * 299 + skyColor.green() * 587 + skyColor.blue() * 114) / 1000;
    QColor textColor = (isNightTime) ? QColor(0xfb, 0xf7, 0xff) : QColor(0x2a, 0x34, 0x3a);
    QColor secondaryTextColor = (isNightTime) ? QColor(0xa9, 0xb3, 0xbb) : QColor(0x72, 0x7c, 0x83);
    QColor tertiaryTextColor = (isNightTime) ? QColor(0xa9, 0xb3, 0xbb) : QColor(0x56, 0x61, 0x67);
    
    // Button text adapts to sky brightness
    QColor buttonTextColor = (isNightTime) ? QColor(0xfb, 0xf7, 0xff) : QColor(0x56, 0x61, 0x67);
    QColor removeButtonTextColor = (isNightTime) ? QColor(0xa9, 0xb3, 0xbb) : QColor(0xa9, 0xb3, 0xbb);
    
    // Calculate hover color (slightly lighter)
    QColor hoverColor = buttonColor.lighter(110);
    QColor pressedColor = buttonColor.darker(110);
    
    dateTimeLabel->setStyleSheet(
        QString("QLabel {"
        "    color: %1;"
        "    padding: 2px 4px 0px 4px;"
        "    margin-bottom: 0px;"
        "}").arg(textColor.name())
    );
    timeZoneLabel->setStyleSheet(
        QString("QLabel {"
        "    color: %1;"
        "    padding: 0px 2px 0px 2px;"
        "    margin-top: 0px;"
        "    margin-bottom: 8px;"
        "}").arg(secondaryTextColor.name())
    );
    dayOffsetLabel->setStyleSheet(
        QString("QLabel {"
        "    color: %1;"
        "    padding: 4px;"
        "    margin-top: 4px;"
        "}").arg(tertiaryTextColor.name())
    );
    nameEdit->setTextColor(textColor);
    
    format24Button->setStyleSheet(
        QString("QToolButton {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 11px;"
        "    font-weight: 500;"
        "}"
        "QToolButton:hover {"
        "    background-color: %3;"
        "}"
        "QToolButton:pressed {"
        "    background-color: %4;"
        "}")
        .arg(buttonColor.name())
        .arg(buttonTextColor.name())
        .arg(hoverColor.name())
        .arg(pressedColor.name())
    );
    
    globeButton->setStyleSheet(
        QString("QToolButton {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 14px;"
        "}"
        "QToolButton:hover {"
        "    background-color: %3;"
        "}"
        "QToolButton:pressed {"
        "    background-color: %4;"
        "}")
        .arg(buttonColor.name())
        .arg(buttonTextColor.name())
        .arg(hoverColor.name())
        .arg(pressedColor.name())
    );
    
    removeButton->setStyleSheet(
        QString("QPushButton {"
        "    background-color: transparent;"
        "    color: %1;"
        "    border: none;"
        "    border-radius: 12px;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %2;"
        "    color: %1;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %3;"
        "}")
        .arg(removeButtonTextColor.name())
        .arg(hoverColor.name())
        .arg(pressedColor.name())
    );
}

QColor TimeZoneWidget::calculateSkyColor(const QDateTime &localTime) const{
    int hour = localTime.time().hour();
    int minute = localTime.time().minute();
    double timeInHours = hour + (minute / 60.0);
    
    QColor color;
    
    // Night: 8 PM to 5 AM (dark blue)
    if (timeInHours >= 20.0 || timeInHours < 5.0){
        color = QColor(30, 40, 60);
    }
    // Dawn transition: 5 AM to 10 AM (night fading to morning blue)
    else if (timeInHours >= 5.0 && timeInHours < 10.0){
        double progress = (timeInHours - 5.0) / 5.0;
        QColor night(30, 40, 60);
        QColor morning(210, 230, 255);
        color = QColor(
            night.red() + progress * (morning.red() - night.red()),
            night.green() + progress * (morning.green() - night.green()),
            night.blue() + progress * (morning.blue() - night.blue())
        );
    }
    // Late morning to noon: 10 AM to 12 PM (blue to yellow)
    else if (timeInHours >= 10.0 && timeInHours < 12.0){
        double progress = (timeInHours - 10.0) / 2.0;
        QColor morning(200, 220, 255);
        QColor noon(255, 255, 220);
        color = QColor(
            morning.red() + progress * (noon.red() - morning.red()),
            morning.green() + progress * (noon.green() - morning.green()),
            morning.blue() + progress * (noon.blue() - morning.blue())
        );
    }
    // Noon to afternoon: 12 PM to 2 PM (yellow at peak)
    else if (timeInHours >= 12.0 && timeInHours < 14.0){
        color = QColor(255, 255, 220);
    }
    // Afternoon: 2 PM to 5 PM (yellow back to blue)
    else if (timeInHours >= 14.0 && timeInHours < 17.0){
        double progress = (timeInHours - 14.0) / 3.0;
        QColor noon(255, 255, 220);
        QColor afternoon(200, 220, 255);
        color = QColor(
            noon.red() + progress * (afternoon.red() - noon.red()),
            noon.green() + progress * (afternoon.green() - noon.green()),
            noon.blue() + progress * (afternoon.blue() - noon.blue())
        );
    }
    // Evening to sunset: 5 PM to 7 PM (blue to reddish sunset)
    else if (timeInHours >= 17.0 && timeInHours < 19.0){
        double progress = (timeInHours - 17.0) / 2.0;
        QColor afternoon(200, 220, 255);
        QColor sunset(255, 140, 120);
        color = QColor(
            afternoon.red() + progress * (sunset.red() - afternoon.red()),
            afternoon.green() + progress * (sunset.green() - afternoon.green()),
            afternoon.blue() + progress * (sunset.blue() - afternoon.blue())
        );
    }
    // Dusk: 7 PM to 8 PM (sunset to night)
    else if (timeInHours >= 19.0 && timeInHours < 20.0){
        double progress = (timeInHours - 19.0) / 1.0;
        QColor sunset(255, 140, 120);
        QColor night(30, 40, 60);
        color = QColor(
            sunset.red() + progress * (night.red() - sunset.red()),
            sunset.green() + progress * (night.green() - sunset.green()),
            sunset.blue() + progress * (night.blue() - sunset.blue())
        );
    }
    else{
        color = QColor(255, 255, 255);
    }
    
    // Blend 20% with white to reduce opacity/intensity
    int r = color.red() * 0.8 + 255 * 0.2;
    int g = color.green() * 0.8 + 255 * 0.2;
    int b = color.blue() * 0.8 + 255 * 0.2;
    
    return QColor(r, g, b);
}

double TimeZoneWidget::calculateSunPosition(const QDateTime &localTime) const{
    Q_UNUSED(localTime);
    return 0.0;
}

int TimeZoneWidget::timestampToSliderValue(qint64 timestamp) const
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp, currentTimeZone);
    
    int hour = dt.time().hour();
    int minute = dt.time().minute();
    
    int quarterHours = (hour * 4) + (minute / 15);
    
    return quarterHours;
}

qint64 TimeZoneWidget::sliderValueToTimestamp(int value) const
{
    int quarterHours = value;
    
    QDateTime dt = QDateTime::fromSecsSinceEpoch(baseTimestamp, currentTimeZone);
    QDate currentDate = dt.date();
    
    int hour = (quarterHours / 4) % 24;
    int minute = (quarterHours % 4) * 15;
    int dayOffset = quarterHours / 96;
    
    QDate newDate = currentDate.addDays(dayOffset);
    QTime newTime(hour, minute, 0);
    
    QDateTime newDateTime(newDate, newTime, currentTimeZone);
    
    return newDateTime.toSecsSinceEpoch();
}

QString TimeZoneWidget::getFriendlyName() const
{
    return nameEdit->text();
}

void TimeZoneWidget::setFriendlyName(const QString &name)
{
    nameEdit->setText(name);
}

QString TimeZoneWidget::getTimeZoneId() const
{
    return QString::fromUtf8(currentTimeZone.id());
}

void TimeZoneWidget::setTimeZoneId(const QString &tzId)
{
    currentTimeZone = QTimeZone(tzId.toUtf8());
    
    int index = timeZoneCombo->findData(tzId.toUtf8());
    
    if (index != -1){
        updatingInternally = true;
        timeZoneCombo->setCurrentIndex(index);
        updatingInternally = false;
    }
    
    int sliderValue = timestampToSliderValue(baseTimestamp);
    updatingInternally = true;
    timeSlider->setValue(sliderValue);
    updatingInternally = false;
    
    updateDisplay();
}

bool TimeZoneWidget::getIs24HourFormat() const
{
    return is24HourFormat;
}

void TimeZoneWidget::setIs24HourFormat(bool is24Hour)
{
    is24HourFormat = is24Hour;
    format24Button->setText(is24Hour ? "24" : "12");
    updateDisplay();
    updateSliderLabels();
}

void TimeZoneWidget::selectName(){
    nameEdit->selectAll();
}

void TimeZoneWidget::reloadSettings(){
    QSettings settings("UnfuckMyTimeZoneMath", "UnfuckMyTimeZoneMath");
    skyColorEnabled = settings.value("appearance/skyColor", true).toBool();
    updateDisplay();
}

void TimeZoneWidget::updateSliderLabels(){
    for (int hour = 0; hour <= 24; hour++){
        QString label;
        
        if (is24HourFormat){
            label = QString("%1").arg(hour, 2, 10, QChar('0'));
        }
        else{
            if (hour == 0){
                label = "12a";
            }
            else if (hour < 12)
            {
                label = QString::number(hour) + "a";
            }
            else if (hour == 12)
            {
                label = "12p";
            }
            else{
                label = QString::number(hour - 12) + "p";
            }
        }
        
        hourLabels[hour]->setText(label);
    }
}

void TimeZoneWidget::mousePressEvent(QMouseEvent *event)
{
    if (nameEdit && nameEdit->isEditing()){
        QRect editRect = nameEdit->geometry();
        if (!editRect.contains(event->pos())){
            nameEdit->finishEditing();
        }
    }
    
    if (event->button() == Qt::LeftButton){
        dragStartPosition = event->pos();
    }

    QWidget::mousePressEvent(event);
}

void TimeZoneWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)){
        QWidget::mouseMoveEvent(event);
        return;
    }

    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()){
        QWidget::mouseMoveEvent(event);
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    mimeData->setData("application/x-timezonewidget", QByteArray::number(reinterpret_cast<quintptr>(this)));
    drag->setMimeData(mimeData);

    QGraphicsColorizeEffect *grayscaleEffect = new QGraphicsColorizeEffect();
    grayscaleEffect->setColor(QColor(128, 128, 128));
    grayscaleEffect->setStrength(1.0);
    setGraphicsEffect(grayscaleEffect);

    emit dragStarted(this);

    drag->exec(Qt::MoveAction);

    setGraphicsEffect(nullptr);
    updateDisplay();
}

void TimeZoneWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-timezonewidget")){
        TimeZoneWidget *source = reinterpret_cast<TimeZoneWidget*>(event->mimeData()->data("application/x-timezonewidget").toULongLong());
        
        if (source != this){
            event->acceptProposedAction();
            showDropIndicator(event->position().toPoint());
        }
    }
}

void TimeZoneWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-timezonewidget")){
        TimeZoneWidget *source = reinterpret_cast<TimeZoneWidget*>(event->mimeData()->data("application/x-timezonewidget").toULongLong());
        
        if (source != this){
            event->acceptProposedAction();
            showDropIndicator(event->position().toPoint());
        }
    }
}

void TimeZoneWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    hideDropIndicator();
}

void TimeZoneWidget::dropEvent(QDropEvent *event)
{
    hideDropIndicator();

    if (event->mimeData()->hasFormat("application/x-timezonewidget")){
        TimeZoneWidget *source = reinterpret_cast<TimeZoneWidget*>(event->mimeData()->data("application/x-timezonewidget").toULongLong());

        if (source != this){
            emit dropReceived(this, source);
            event->acceptProposedAction();
        }
    }
}

void TimeZoneWidget::showDropIndicator(const QPoint &pos)
{
    int widgetCenter = width() / 2;
    
    if (pos.x() < widgetCenter){
        dropIndicator->setGeometry(-2, 0, 4, height());
    }
    else{
        dropIndicator->setGeometry(width() - 2, 0, 4, height());
    }

    dropIndicator->raise();
    dropIndicator->show();
}

void TimeZoneWidget::hideDropIndicator(){
    dropIndicator->hide();
}
