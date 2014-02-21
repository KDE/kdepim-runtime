/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *   Copyright (C) 2014 by Heena Mahour <heena393@gmail.com>
 */

#ifndef TODOMODEL_H
#define TODOMODEL_H

#include <akonadi/monitor.h>
#include <akonadi/collection.h>

#include <kcalcore/event.h>
#include <kcalcore/todo.h>

#include <KUrl>
#include <QStandardItemModel>
#include <QColor>
#include <QHash>
#include <QString>

class QStandardItem;
class KJob;

static const int ShortDateFormat = 0;
static const int LongDateFormat = 1;
static const int FancyShortDateFormat = 2;
static const int FancyLongDateFormat = 3;
static const int CustomDateFormat = 4;

static const int urgentColorPos = 0;
static const int passedColorPos = 1;
static const int todoColorPos = 2;
static const int finishedTodoColorPos = 3;

class TodoModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum EventRole {
        SortRole = Qt::UserRole + 1,
        UIDRole,
        ItemTypeRole,
        TooltipRole,
        ItemIDRole,
        CollectionRole
    };

    enum ItemType {
        HeaderItem = 0,
        NormalItem,
        BirthdayItem,
        AnniversaryItem,
        TodoItem
    };

    explicit TodoModel(QObject *parent = 0, int urgencyTime = 15, int birthdayTime = 14, QList<QColor> colorList = QList<QColor>(), int count = 0, bool autoGroupHeader = false);
    ~TodoModel();

public:
    void setDateFormat(int format, QString string);
    void setCategoryColors(const QHash<QString, QColor>);
    void setHeaderItems(QStringList headerParts);
    void initModel();
    void initMonitor();
    void resetModel();
    void settingsChanged(int urgencyTime, int birthdayTime, QList<QColor> itemColors, int count, bool autoGroupHeader);
    QMap<QString, QString> usedCollections();

private slots:
    void initialCollectionFetchFinished(KJob *);
    void initialItemFetchFinished(KJob *);
    void addEventItem(const QMap <QString, QVariant> &values);
    void addTodoItem(const QMap <QString, QVariant> &values);
    void itemAdded(const Akonadi::Item &, const Akonadi::Collection &);
    void removeItem(const Akonadi::Item &);
    void itemChanged(const Akonadi::Item &, const QSet<QByteArray> &);
    void itemMoved(const Akonadi::Item &, const Akonadi::Collection &, const Akonadi::Collection &);

private:
    void createHeaderItems(QStringList headerParts);
    void initHeaderItem(QStandardItem *item, QString title, QString toolTip, int days);
    void addItem(const Akonadi::Item &item, const Akonadi::Collection &collection);
    void addItemRow(QDate eventDate, QStandardItem *items);
    QMap<QString, QVariant> eventDetails(const Akonadi::Item &, KCalCore::Event::Ptr);
    QMap<QString, QVariant> todoDetails(const Akonadi::Item &, KCalCore::Todo::Ptr);

private:
    QStandardItem *parentItem;
    QStringList m_headerPartsList;
    QMap<QDate, QStandardItem *> m_sectionItemsMap;
    QMap<QString, QString> m_usedCollections;
    int urgency, birthdayUrgency, recurringCount;
    QColor urgentBg, passedFg, todoBg, finishedTodoBg;
    QHash<QString, QColor> m_categoryColors;
    QHash<Akonadi::Entity::Id, Akonadi::Collection> m_collections;
    QList<Akonadi::Entity::Id> itemIds;
    Akonadi::Monitor *m_monitor;
    bool useAutoGroupHeader;

signals:
    void modelNeedsExpanding();
};

#endif
