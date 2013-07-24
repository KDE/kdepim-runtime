#include "calendardayhelper.h"

#include <QDebug>

CalendarDayHelper::CalendarDayHelper(QObject *parent)
    : QObject(parent)
    , m_year(0)
    , m_month(0)
{
    m_cal = new Akonadi::ETMCalendar();
    m_cal->setCollectionFilteringEnabled(false);

    connect(m_cal, SIGNAL(calendarChanged()), this, SLOT(fillLists()));
    connect(m_cal, SIGNAL(calendarChanged()), this, SIGNAL(calendarChanged()));

//    QTreeView* tv = new QTreeView();
//    tv->setModel(m_cal->model());
//    tv->show();
}

void CalendarDayHelper::setDate(int year, int month)
{
    if(m_year != year) {
        m_year = year;
    }

    if(m_month != month) {
        m_month = month;
    }
}

bool CalendarDayHelper::containsHolidayItems(int day)
{
    // Not implemented yet
    return false;
}

bool CalendarDayHelper::containsEventItems(int day)
{
    QDate compareDate(m_year, m_month, day);
    qDebug() << "Going to check events against a list with:" << m_eventList.count() << "items";
    foreach(KCalCore::Event::Ptr event, m_eventList) {
        qDebug() << "Checking entry against date:" << compareDate << "with dtStart:" << event->dtStart().date() << "and dtEnd:" << event->dtEnd().date();

        // Keep this line as comment for now. This line works for overlapping calendar events as well.
        if(event->dtStart().date() <= compareDate && event->dtEnd().date() >= compareDate) {

        // This condition only enters when the event started todat
     //  if(event->dtStart().date() == compareDate) {
            return true;
        }
    }
    return false;
}

bool CalendarDayHelper::containsTodoItems(int day)
{
    QDate compareDate(m_year, m_month, day);
    foreach(KCalCore::Todo::Ptr todo, m_todoList) {
        if(todo->dtStart().date() == compareDate ) {
            return true;
        }
    }
    return false;
}

bool CalendarDayHelper::containsJournalItems(int day)
{
  // Not implemented yet
  return false;
}

void CalendarDayHelper::fillLists()
{
    QDate date(m_year, m_month, 1);
    QDate endDate = QDate(m_year, m_month, date.daysInMonth());
    m_eventList = m_cal->rawEvents(date, endDate);
    m_todoList = m_cal->rawTodos(date, endDate);

}
