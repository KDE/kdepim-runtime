#ifndef CALENDARDAYHELPER_H
#define CALENDARDAYHELPER_H

#include <QObject>
#include <akonadi/calendar/etmcalendar.h>
#include <kcalcore/event.h>

#include "selecteddayfilter.h"

class QAbstractItemModel;

class CalendarDayHelper : public QObject
{
    Q_OBJECT
public:
    explicit CalendarDayHelper(QObject *parent = 0);
    
    void setDate(int year, int month);

    bool containsHolidayItems(int day);
    bool containsEventItems(int day);
    bool containsTodoItems(int day);
    bool containsJournalItems(int day);

    void updateCurrentDatModel();
    QAbstractItemModel* selectedDayModel() const;

signals:
    void calendarChanged();
    
public slots:
    void fillLists();

private:
    Akonadi::ETMCalendar * m_cal;
    int m_year;
    int m_month;
    KCalCore::Event::List m_eventList;
    KCalCore::Todo::List m_todoList;
    KCalCore::Journal::List m_journalList;
};

#endif // CALENDARDAYHELPER_H
