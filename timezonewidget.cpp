#include "timezonewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <algorithm>

TimeZoneWidget::TimeZoneWidget(QWidget *parent)
    : QWidget(parent),
      baseTimestamp(QDateTime::currentSecsSinceEpoch()),
      currentTimeZone(QTimeZone::systemTimeZone()),
      is24HourFormat(true),
      updatingInternally(false)
{
    setupUI();
    populateTimeZones();
    updateDisplay();
    updateSliderLabels();
}

void TimeZoneWidget::setupUI()
{
    setFixedWidth(210);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    QFrame *frame = new QFrame();
    frame->setFrameStyle(QFrame::Box | QFrame::Raised);
    frame->setLineWidth(1);
    
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(5, 5, 5, 5);
    frameLayout->setSpacing(3);
    
    nameEdit = new QLineEdit("My Wonderful Self");
    nameEdit->setPlaceholderText("Friendly Name");
    connect(nameEdit, &QLineEdit::textChanged, this, &TimeZoneWidget::widgetModified);
    frameLayout->addWidget(nameEdit);
    
    dateTimeLabel = new QLabel("Wed, 12:00a");
    dateTimeLabel->setAlignment(Qt::AlignCenter);
    QFont dtFont = dateTimeLabel->font();
    dtFont.setPointSize(11);
    dtFont.setBold(true);
    dateTimeLabel->setFont(dtFont);
    frameLayout->addWidget(dateTimeLabel);
    
    dayOffsetLabel = new QLabel("+0 Days");
    dayOffsetLabel->setAlignment(Qt::AlignCenter);
    QFont offsetFont = dayOffsetLabel->font();
    offsetFont.setPointSize(9);
    dayOffsetLabel->setFont(offsetFont);
    frameLayout->addWidget(dayOffsetLabel);
    
    timeSlider = new QSlider(Qt::Vertical);
    timeSlider->setMinimum(0);
    timeSlider->setMaximum(96);
    timeSlider->setValue(48);
    timeSlider->setTickPosition(QSlider::TicksRight);
    timeSlider->setTickInterval(4);
    timeSlider->setMinimumHeight(480);
    connect(timeSlider, &QSlider::valueChanged, this, &TimeZoneWidget::onSliderValueChanged);
    
    sliderContainer = new QWidget();
    QHBoxLayout *sliderContainerLayout = new QHBoxLayout(sliderContainer);
    sliderContainerLayout->setContentsMargins(0, 0, 0, 0);
    sliderContainerLayout->setSpacing(3);
    
    QWidget *labelsWidget = new QWidget();
    labelsWidget->setFixedWidth(28);
    QVBoxLayout *labelsLayout = new QVBoxLayout(labelsWidget);
    labelsLayout->setContentsMargins(0, 0, 0, 0);
    labelsLayout->setSpacing(0);
    
    for (int i = 0; i <= 24; i++)
    {
        QLabel *hourLabel = new QLabel();
        hourLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        hourLabel->setFixedHeight(19);
        QFont labelFont = hourLabel->font();
        labelFont.setPointSize(7);
        hourLabel->setFont(labelFont);
        hourLabels.append(hourLabel);
        labelsLayout->addWidget(hourLabel);
        
        if (i < 24)
        {
            labelsLayout->addSpacing(0);
        }
    }
    
    sliderContainerLayout->addWidget(labelsWidget);
    sliderContainerLayout->addWidget(timeSlider);
    
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    sliderLayout->addStretch();
    sliderLayout->addWidget(sliderContainer);
    sliderLayout->addStretch();
    frameLayout->addLayout(sliderLayout);
    
    timeZoneCombo = new QComboBox();
    connect(timeZoneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &TimeZoneWidget::onTimeZoneChanged);
    frameLayout->addWidget(timeZoneCombo);
    
    format24HourCheck = new QCheckBox("24-hour format");
    format24HourCheck->setChecked(true);
    connect(format24HourCheck, &QCheckBox::checkStateChanged, this, &TimeZoneWidget::onFormatChanged);
    frameLayout->addWidget(format24HourCheck);
    
    removeButton = new QPushButton("Remove");
    connect(removeButton, &QPushButton::clicked, this, [this]()
    {
        emit removeRequested(this);
    });
    frameLayout->addWidget(removeButton);
    
    mainLayout->addWidget(frame);
}

void TimeZoneWidget::populateTimeZones()
{
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
    
    for (const TimeZoneEntry &entry : majorCities)
    {
        if (entry.isUS)
        {
            usCities.append(entry);
        }
        else
        {
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
    
    for (const TimeZoneEntry &entry : usCities)
    {
        timeZoneCombo->addItem(entry.displayName, entry.tzId);
    }
    
    timeZoneCombo->insertSeparator(timeZoneCombo->count());
    
    for (const TimeZoneEntry &entry : otherCities)
    {
        timeZoneCombo->addItem(entry.displayName, entry.tzId);
    }
    
    QByteArray systemTzId = QTimeZone::systemTimeZoneId();
    int index = timeZoneCombo->findData(systemTzId);
    
    if (index != -1)
    {
        timeZoneCombo->setCurrentIndex(index);
    }
}

void TimeZoneWidget::setBaseTimestamp(qint64 timestamp)
{
    if (baseTimestamp == timestamp)
    {
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

void TimeZoneWidget::updateDisplay()
{
    QDateTime baseDateTime = QDateTime::fromSecsSinceEpoch(baseTimestamp, QTimeZone::utc());
    QDateTime localDateTime = baseDateTime.toTimeZone(currentTimeZone);
    
    QString dayFormat = "ddd";
    QString timeFormat = is24HourFormat ? "HH:mm" : "h:mma";
    QString displayFormat = dayFormat + ", " + timeFormat;
    
    dateTimeLabel->setText(localDateTime.toString(displayFormat));
    
    QDateTime referenceDateTime = QDateTime::fromSecsSinceEpoch(baseTimestamp, QTimeZone::utc());
    QDateTime referenceLocal = referenceDateTime.toTimeZone(QTimeZone::systemTimeZone());
    QDateTime widgetLocal = referenceDateTime.toTimeZone(currentTimeZone);
    
    int refDay = referenceLocal.date().toJulianDay();
    int widgetDay = widgetLocal.date().toJulianDay();
    int dayOffset = widgetDay - refDay;
    
    if (dayOffset == 0)
    {
        dayOffsetLabel->setText("Same Day");
    }
    else if (dayOffset > 0)
    {
        dayOffsetLabel->setText(QString("+%1 Day%2").arg(dayOffset).arg(dayOffset > 1 ? "s" : ""));
    }
    else
    {
        dayOffsetLabel->setText(QString("%1 Day%2").arg(dayOffset).arg(dayOffset < -1 ? "s" : ""));
    }
}

void TimeZoneWidget::onSliderValueChanged(int value)
{
    if (updatingInternally)
    {
        return;
    }
    
    baseTimestamp = sliderValueToTimestamp(value);
    updateDisplay();
    
    emit timeChanged(baseTimestamp);
}

void TimeZoneWidget::onTimeZoneChanged(int index)
{
    if (index < 0 || updatingInternally)
    {
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

int TimeZoneWidget::timestampToSliderValue(qint64 timestamp) const
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp, currentTimeZone);
    
    int hour = dt.time().hour();
    int minute = dt.time().minute();
    
    int quarterHours = (hour * 4) + (minute / 15);
    
    return 96 - quarterHours;
}

qint64 TimeZoneWidget::sliderValueToTimestamp(int value) const
{
    int quarterHours = 96 - value;
    
    int hour = quarterHours / 4;
    int minute = (quarterHours % 4) * 15;
    
    QDateTime dt = QDateTime::fromSecsSinceEpoch(baseTimestamp, currentTimeZone);
    QTime newTime(hour, minute, 0);
    dt.setTime(newTime);
    
    return dt.toSecsSinceEpoch();
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
    
    if (index != -1)
    {
        updatingInternally = true;
        timeZoneCombo->setCurrentIndex(index);
        updatingInternally = false;
    }
    
    updateDisplay();
}

bool TimeZoneWidget::getIs24HourFormat() const
{
    return is24HourFormat;
}

void TimeZoneWidget::setIs24HourFormat(bool is24Hour)
{
    is24HourFormat = is24Hour;
    format24HourCheck->setChecked(is24Hour);
    updateDisplay();
    updateSliderLabels();
}

void TimeZoneWidget::selectName()
{
    nameEdit->setFocus();
    nameEdit->selectAll();
}

void TimeZoneWidget::updateSliderLabels()
{
    for (int hour = 0; hour <= 24; hour++)
    {
        QString label;
        
        if (is24HourFormat)
        {
            label = QString("%1").arg(hour, 2, 10, QChar('0'));
        }
        else
        {
            if (hour == 0)
            {
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
            else
            {
                label = QString::number(hour - 12) + "p";
            }
        }
        
        hourLabels[hour]->setText(label);
    }
}
