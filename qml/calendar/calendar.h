#ifndef CALENDAR_H
#define CALENDAR_H

#include <QObject>
#include <QDate>
#include <QAbstractListModel>
#include <kcalendarsystem.h>

#include "daydata.h"
#include "daysmodel.h"
#include "calendardayhelper.h"

class Calendar : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    Q_PROPERTY(int types READ types WRITE setTypes NOTIFY typesChanged)
    Q_PROPERTY(int days READ days WRITE setDays NOTIFY daysChanged)
    Q_PROPERTY(int weeks READ weeks WRITE setWeeks NOTIFY weeksChanged)
    Q_PROPERTY(int startDay READ startDay WRITE setStartDay NOTIFY startDayChanged)
    Q_PROPERTY(int year READ year NOTIFY yearChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString monthName READ monthName NOTIFY monthNameChanged)
    Q_PROPERTY(QAbstractListModel* daysModel READ daysModel CONSTANT)

    Q_ENUMS(Type)

public:
    enum Type {
        Holiday = 1,
        Event = 2,
        Todo = 4,
        Journal = 8
    };
    Q_DECLARE_FLAGS(Types, Type)


    explicit Calendar(QObject *parent = 0);

    // Start date
    QDate startDate() const;
    void setStartDate(const QDate &dateTime);

    // Types
    int types() const;
    void setTypes(int types);

    // Days
    int days();
    void setDays(int days);

    // Weeks
    int weeks();
    void setWeeks(int weeks);

    // Start day
    int startDay();
    void setStartDay(int day);

    // Error message
    QString errorMessage() const;

    // Month name
    QString monthName() const;
    int year() const;
 
    // Models
    QAbstractListModel* daysModel() const;


    // QML invokables
    Q_INVOKABLE void next();
    Q_INVOKABLE void nextyear();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void previousyear();
    Q_INVOKABLE QString dayName(int weekDay) ;
    
signals:
    void startDateChanged();
    void typesChanged();
    void daysChanged();
    void weeksChanged();
    void startDayChanged();
    void errorMessageChanged();
    void monthNameChanged();
    void yearChanged();

public slots:
    void updateData();
    
private:
    QDate m_startDate;
    Types m_types;
    QList<DayData> m_dayList;
    DaysModel* m_daysModel;
    CalendarDayHelper* m_dayHelper;
    int m_days;
    int m_weeks;
    int m_startDay;
    QString m_errorMessage;
    
};

#endif // CALENDAR_H
