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
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)

    Q_ENUMS(Type)

public:
    enum Type {
        Holiday = 1,
        Event = 2,
        Todo = 4,
        Journal = 8
    };
    Q_DECLARE_FLAGS(Types, Type)

    explicit CalendarData(QObject *parent = 0);
    QDate startDate() const;
    void setStartDate(const QDate &dateTime);
    QDate endDate() const;
    void setEndDate(const QDate &dateTime);
    QAbstractItemModel* model() const;


signals:
    void startDateChanged();
    void endDateChanged();
    void typesChanged();
    void errorMessageChanged();
    void loadingChanged();

private:
    int types() const;
    void setTypes(int types);
    QString errorMessage() const;
    bool loading() const;

    void updateTypes();

    QDate m_startDate;
    QDate m_endDate;
    Types m_types;

    Akonadi::ETMCalendar *m_etmCalendar;
    Akonadi::EntityMimeTypeFilterModel *m_itemList;
    DateTimeRangeFilterModel *m_filteredList;
};

#endif // CALENDARDATA_H
