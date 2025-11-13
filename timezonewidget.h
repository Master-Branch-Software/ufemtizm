#ifndef TIMEZONEWIDGET_H
#define TIMEZONEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDateTime>
#include <QTimeZone>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

class TimeZoneWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeZoneWidget(QWidget *parent = nullptr);
    
    void setBaseTimestamp(qint64 timestamp);
    qint64 getBaseTimestamp() const;
    void updateDisplay();
    
    QString getFriendlyName() const;
    void setFriendlyName(const QString &name);
    QString getTimeZoneId() const;
    void setTimeZoneId(const QString &tzId);
    bool getIs24HourFormat() const;
    void setIs24HourFormat(bool is24Hour);
    void selectName();

signals:
    void timeChanged(qint64 baseTimestamp);
    void removeRequested(TimeZoneWidget *widget);
    void widgetModified();
    void dragStarted(TimeZoneWidget *widget);
    void dropReceived(TimeZoneWidget *target, TimeZoneWidget *source);

private slots:
    void onSliderValueChanged(int value);
    void onTimeZoneChanged(int index);
    void onFormatChanged(int state);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();
    void populateTimeZones();
    void updateSliderLabels();
    void showDropIndicator(const QPoint &pos);
    void hideDropIndicator();
    int timestampToSliderValue(qint64 timestamp) const;
    qint64 sliderValueToTimestamp(int value) const;
    
    QLineEdit *nameEdit;
    QLabel *dateTimeLabel;
    QLabel *dayOffsetLabel;
    QSlider *timeSlider;
    QWidget *sliderContainer;
    QVector<QLabel*> hourLabels;
    QComboBox *timeZoneCombo;
    QCheckBox *format24HourCheck;
    QPushButton *removeButton;
    
    qint64 baseTimestamp;
    QTimeZone currentTimeZone;
    bool is24HourFormat;
    bool updatingInternally;
    QPoint dragStartPosition;
    QFrame *frame;
    QFrame *dropIndicator;
};

#endif // TIMEZONEWIDGET_H
