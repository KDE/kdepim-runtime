#ifndef DATETIMERANGEFILTERMODEL_H
#define DATETIMERANGEFILTERMODEL_H

#include <QDate>
#include <QSortFilterProxyModel>

class DateTimeRangeFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DateTimeRangeFilterModel(QObject *parent = 0);

    void setStartDate(const QDate &date);
    void setEndDate(const QDate &date);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    QDate m_startDate;
    QDate m_endDate;
    //QDate test;
};

#endif
