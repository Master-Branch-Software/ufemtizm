#ifndef TIMEZONEWIDGET_HPP
#define TIMEZONEWIDGET_HPP

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
#include <QResizeEvent>
#include <QToolButton>
#include <QButtonGroup>

class QGraphicsDropShadowEffect;

class EditableLabel : public QWidget
{
    Q_OBJECT

public:
    explicit EditableLabel(const QString &text = "", QWidget *parent = nullptr);
    QString text() const;
    void setText(const QString &text);
    void selectAll();
    bool isEditing() const;
    void finishEditing();
    void setTextColor(const QColor &color);

signals:
    void textChanged(const QString &text);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *mLabel;
    QLineEdit *mLineEdit;
};

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
    void reloadSettings();

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
    void updateTimeZoneLabel();
    void updateSkyColor();
    void applyHourLabelColors(const QColor &color);
    void applySliderStyle(bool nightStyle);
    QColor calculateSkyColor(const QDateTime &localTime) const;
    double calculateSunPosition(const QDateTime &localTime) const;
    
    EditableLabel *nameEdit;
    QLabel *dateTimeLabel;
    QLabel *timeZoneLabel;
    QLabel *dayOffsetLabel;
    QSlider *timeSlider;
    QWidget *sliderContainer;
    QVector<QLabel*> hourLabels;
    QComboBox *timeZoneCombo;
    QToolButton *format24Button;
    QToolButton *globeButton;
    QPushButton *removeButton;
    
    qint64 baseTimestamp;
    QTimeZone currentTimeZone;
    bool is24HourFormat;
    bool updatingInternally;
    bool skyColorEnabled;
    QPoint dragStartPosition;
    QFrame *frame;
    QFrame *dropIndicator;
    QGraphicsDropShadowEffect *frameShadow;

    QColor lastHourLabelColor;
    int lastSliderMode;
};

#endif // TIMEZONEWIDGET_HPP
