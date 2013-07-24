#ifndef CALENDARDATA_H
#define CALENDARDATA_H

#include <QObject>
#include <QFlags>
#include <QDate>

namespace Akonadi {
class EntityMimeTypeFilterModel;
class ETMCalendar;
}

class DateTimeRangeFilterModel;
class QAbstractItemModel;

class CalendarData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    Q_PROPERTY(QDate endDate READ endDate WRITE setEndDate NOTIFY endDateChanged)
    Q_PROPERTY(int types READ types WRITE setTypes NOTIFY typesChanged)
    Q_PROPERTY(int sorting READ sorting WRITE setSorting NOTIFY sortingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)

    Q_ENUMS(Type)
    Q_ENUMS(Sort)

public:
    enum Type {
        Holiday = 1,
        Event = 2,
        Todo = 4,
        Journal = 8
    };
    Q_DECLARE_FLAGS(Types, Type)

    enum Sort {
        Ascending = 1,
        Descending = 2,
        None = 4
    };
    Q_DECLARE_FLAGS(Sorting, Sort)

    explicit CalendarData(QObject *parent = 0);
    QDate startDate() const;
    void setStartDate(const QDate &dateTime);
    QDate endDate() const;
    void setEndDate(const QDate &dateTime);
    QAbstractItemModel* model() const;

    // Sorting
    int sorting() const;
    void setSorting(int sorting);


signals:
    void startDateChanged();
    void endDateChanged();
    void typesChanged();
    void errorMessageChanged();
    void loadingChanged();
    void sortingChanged();

private:
    int types() const;
    void setTypes(int types);
    QString errorMessage() const;
    bool loading() const;

    void updateTypes();

    QDate m_startDate;
    QDate m_endDate;
    Types m_types;
    Sorting m_sorting;

    Akonadi::ETMCalendar *m_etmCalendar;
    Akonadi::EntityMimeTypeFilterModel *m_itemList;
    DateTimeRangeFilterModel *m_filteredList;
};

#endif // CALENDARDATA_H
