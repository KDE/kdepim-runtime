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

#include "todomodel.h"

#include <kcalcore/recurrence.h>
#include <kcalutils/incidenceformatter.h>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/servermanager.h>

#include <QDate>
#include <QStandardItem>
#include <KIcon>
#include <KGlobal>
#include <KLocale>
#include <KGlobalSettings>
#include <KDateTime>

#include <Plasma/Theme>

#include <KDebug>

TodoModel::TodoModel(QObject *parent, int urgencyTime, int birthdayTime, QList<QColor> colorList, int count, bool autoGroupHeader) : QStandardItemModel(parent),
    parentItem(0),
    m_monitor(0)
{
    parentItem = invisibleRootItem();
    settingsChanged(urgencyTime, birthdayTime, colorList, count, autoGroupHeader);
}

TodoModel::~TodoModel()
{
}

void TodoModel::initModel()
{
    createHeaderItems(m_headerPartsList);

    Akonadi::CollectionFetchScope scope;
    QStringList mimeTypes;
    mimeTypes << KCalCore::Event::eventMimeType();
    mimeTypes << KCalCore::Todo::todoMimeType();
    mimeTypes << "text/calendar";
    scope.setContentMimeTypes(mimeTypes);

    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                                       Akonadi::CollectionFetchJob::Recursive);
    job->setFetchScope(scope);
    connect(job, SIGNAL(result(KJob *)), this, SLOT(initialCollectionFetchFinished(KJob *)));
    job->start();
}

void TodoModel::initialCollectionFetchFinished(KJob *job)
{
    if (job->error()) {
        kDebug() << "Initial collection fetch failed!";
    } else {
        Akonadi::CollectionFetchJob *cJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
        Akonadi::Collection::List collections = cJob->collections();
        foreach (const Akonadi::Collection &collection, collections) {
            m_collections.insert(collection.id(), collection);
            Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(collection);
            job->fetchScope().fetchFullPayload();
            job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
            
            connect(job, SIGNAL(result(KJob *)), this, SLOT(initialItemFetchFinished(KJob *)));
            job->start();
        }
    }
}

void TodoModel::initialItemFetchFinished(KJob *job)
{
    if (job->error()) {
        kDebug() << "Initial item fetch failed!";
    } else {
        Akonadi::ItemFetchJob *iJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
        Akonadi::Item::List items = iJob->items();
        foreach (const Akonadi::Item &item, items) {
            if (itemIds.contains(item.id())) {
                removeItem(item);
            }

            if (item.hasPayload <KCalCore::Event::Ptr>()) {
                KCalCore::Event::Ptr event = item.payload <KCalCore::Event::Ptr>();
                if (event) {
                    addEventItem(eventDetails(item, event));
                    itemIds.append(item.id());
                }
            } else if (item.hasPayload <KCalCore::Todo::Ptr>()) {
                KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
                if (todo) {
                    addTodoItem(todoDetails(item, todo));
                    itemIds.append(item.id());
                }
            }
        }
    }

    setSortRole(TodoModel::SortRole);
    sort(0, Qt::AscendingOrder);
}

void TodoModel::initMonitor()
{
    m_monitor = new Akonadi::Monitor(this);
    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload(true);
    scope.fetchAllAttributes(true);
    m_monitor->fetchCollection(true);
    m_monitor->setItemFetchScope(scope);
    m_monitor->setCollectionMonitored(Akonadi::Collection::root());
    m_monitor->setMimeTypeMonitored(KCalCore::Event::eventMimeType(), true);
    m_monitor->setMimeTypeMonitored(KCalCore::Todo::todoMimeType(), true);
    m_monitor->setMimeTypeMonitored("text/calendar", true);

    connect(m_monitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)),
                       SLOT(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)));
    connect(m_monitor, SIGNAL(itemRemoved(const Akonadi::Item &)),
                       SLOT(removeItem(const Akonadi::Item &)));
    connect(m_monitor, SIGNAL(itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)),
                       SLOT(itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)));
    connect(m_monitor, SIGNAL(itemMoved(const Akonadi::Item &, const Akonadi::Collection &, const Akonadi::Collection &)),
                       SLOT(itemMoved(const Akonadi::Item &, const Akonadi::Collection &, const Akonadi::Collection &)));
}

void TodoModel::initHeaderItem(QStandardItem *item, QString title, QString toolTip, int days)
{
    QMap<QString, QVariant> data;
    QDateTime date = QDateTime(QDate::currentDate().addDays(days));
    data["itemType"] = HeaderItem;
    data["title"] = QString("<b>" + title + "</b>");
    data["date"] = date;
    item->setData(data, Qt::DisplayRole);
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    item->setForeground(QBrush(textColor));
    item->setData(QVariant(date), SortRole);
    item->setData(QVariant(HeaderItem), ItemTypeRole);
    item->setData(QVariant(QString()), CollectionRole);
    item->setData(QVariant(QString()), UIDRole);
    item->setData(QVariant("<qt><b>" + toolTip + "</b></qt>"), TooltipRole);
}

void TodoModel::resetModel()
{
    clear();
    m_sectionItemsMap.clear();
    m_collections.clear();
    m_usedCollections.clear();
    itemIds.clear();
    parentItem = invisibleRootItem();
    delete m_monitor;
    m_monitor = 0;

    if (!Akonadi::ServerManager::isRunning()) {
        QStandardItem *errorItem = new QStandardItem();
        errorItem->setData(QVariant(i18n("Sorry,the Akonadi server is not running!akonadictl start can save you")), Qt::DisplayRole);
        parentItem->appendRow(errorItem);
    } else {
        initModel();
        initMonitor();
    }
}

void TodoModel::settingsChanged(int urgencyTime, int birthdayTime, QList<QColor> itemColors, int count, bool autoGroupHeader)
{
    urgency = urgencyTime;
    birthdayUrgency = birthdayTime;
    urgentBg = itemColors.at(urgentColorPos);
    passedFg = itemColors.at(passedColorPos);
    todoBg = itemColors.at(todoColorPos);
    finishedTodoBg = itemColors.at(finishedTodoColorPos);
    recurringCount = count;
    useAutoGroupHeader = autoGroupHeader;
}

void TodoModel::setCategoryColors(QHash<QString, QColor> categoryColors)
{
    m_categoryColors = categoryColors;
}

void TodoModel::setHeaderItems(QStringList headerParts)
{
    m_headerPartsList = headerParts;
}

void TodoModel::createHeaderItems(QStringList headerParts)
{
    QStandardItem *olderItem = new QStandardItem();
    initHeaderItem(olderItem, i18n("Older todos"), i18n("Unfinished todos"), -28);
    m_sectionItemsMap.insert(olderItem->data(SortRole).toDate(), olderItem);

    QStandardItem *somedayItem = new QStandardItem();
    initHeaderItem(somedayItem, i18n("Due Date not specified - Todos"), i18n("Todos with no due date"), 366);
    m_sectionItemsMap.insert(somedayItem->data(SortRole).toDate(), somedayItem);

    if (!useAutoGroupHeader) {
        for (int i = 0; i < headerParts.size(); i += 3) {
            QStandardItem *item = new QStandardItem();
            initHeaderItem(item, headerParts.value(i), headerParts.value(i + 1), headerParts.value(i + 2).toInt());
            m_sectionItemsMap.insert(item->data(SortRole).toDate(), item);
        }
    }
}

void TodoModel::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    m_collections.insert(collection.id(), collection);
    addItem(item, collection);
}

void TodoModel::removeItem(const Akonadi::Item &item)
{
    foreach (QStandardItem *i, m_sectionItemsMap) {
        QModelIndexList l;
        if (i->hasChildren())
            l = match(i->child(0, 0)->index(), TodoModel::ItemIDRole, item.id(), -1);

        for (int c = l.count(); c > 0; --c) {
            i->removeRow(l.at(c - 1).row());
        }

        int r = i->row();
        if (r != -1 && !i->hasChildren()) {
            takeRow(r);
            emit modelNeedsExpanding();
        }
    }

    itemIds.removeAll(item.id());
}

void TodoModel::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    kDebug() << "item changed";
    removeItem(item);
    addItem(item, item.parentCollection());
}

void TodoModel::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &, const Akonadi::Collection &)
{
    kDebug() << "item moved";
    removeItem(item);
    addItem(item, item.parentCollection());
}

void TodoModel::addItem(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
    
    if (itemIds.contains(item.id())) {
        removeItem(item);
    }

    if (item.hasPayload<KCalCore::Event::Ptr>()) {
        KCalCore::Event::Ptr event = item.payload <KCalCore::Event::Ptr>();
        if (event) {
            addEventItem(eventDetails(item, event));
            itemIds.append(item.id());
        } // if event
    } else if (item.hasPayload <KCalCore::Todo::Ptr>()) {
        KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
        if (todo) {
            addTodoItem(todoDetails(item, todo));
            itemIds.append(item.id());
        }
    }
}

void TodoModel::addEventItem(const QMap<QString, QVariant> &values)
{
    QMap<QString, QVariant> data = values;
    QString category = values["mainCategory"].toString();
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);

    // dont add events starting later than a year
    if (values["startDate"].toDate() > QDate::currentDate().addDays(365)) {
        return;
    }

    if (values["recurs"].toBool()) {
        int c = 0;
        QList<QVariant> dtTimes = values["recurDates"].toList();
        foreach (const QVariant &eventDtTime, dtTimes) {
            if (recurringCount != 0 && c >= recurringCount)
                break;

            QStandardItem *eventItem = new QStandardItem();
            eventItem->setForeground(QBrush(textColor));
            data["startDate"] = eventDtTime;

            int d = values["startDate"].toDateTime().secsTo(values["endDate"].toDateTime());
            data["endDate"] = eventDtTime.toDateTime().addSecs(d);

            QDate itemDt = eventDtTime.toDate();
            if (values["isBirthday"].toBool()) {
                data["itemType"] = BirthdayItem;
                int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
                n > 2000 ? data["yearsSince"] = "XX" : data["yearsSince"] = QString::number(n); // workaround missing facebook birthdays
                if (itemDt >= QDate::currentDate() && QDate::currentDate().daysTo(itemDt) < birthdayUrgency) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else {
                    if (m_categoryColors.contains(i18n("Birthday")) || m_categoryColors.contains("Birthday")) {
                        QString b = "Birthday";
                        if (m_categoryColors.contains(i18n("Birthday"))) {
                            b = i18n("Birthday");
                        }
                        eventItem->setBackground(QBrush(m_categoryColors.value(b)));
                    }
                }
                eventItem->setData(QVariant(BirthdayItem), ItemTypeRole);
            } else if (values["isAnniversary"].toBool()) {
                data["itemType"] = AnniversaryItem;
                int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
                data["yearsSince"] = QString::number(n);
                if (itemDt >= QDate::currentDate() && QDate::currentDate().daysTo(itemDt) < birthdayUrgency) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else {
                    if (m_categoryColors.contains(category)) {
                        eventItem->setBackground(QBrush(m_categoryColors.value(category)));
                    }
                }
                eventItem->setData(QVariant(AnniversaryItem), ItemTypeRole);
            } else {
                data["itemType"] = NormalItem;
                eventItem->setData(QVariant(NormalItem), ItemTypeRole);
                QDateTime itemDtTime = data["startDate"].toDateTime();
                if (itemDtTime > QDateTime::currentDateTime() && QDateTime::currentDateTime().secsTo(itemDtTime) < urgency * 60) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else if (QDateTime::currentDateTime() > itemDtTime) {
                    eventItem->setForeground(QBrush(passedFg));
                } else if (m_categoryColors.contains(category)) {
                    eventItem->setBackground(QBrush(m_categoryColors.value(category)));
                }
            }

            eventItem->setData(data, Qt::DisplayRole);
            eventItem->setData(eventDtTime, SortRole);
            eventItem->setData(values["uid"], UIDRole);
            eventItem->setData(values["itemid"], ItemIDRole);
            eventItem->setData(values["collectionId"], CollectionRole);
            eventItem->setData(values["tooltip"], TooltipRole);

            addItemRow(eventDtTime.toDate(), eventItem);

            ++c;
        }
    } else {
        QStandardItem *eventItem;
        eventItem = new QStandardItem();
        data["itemType"] = NormalItem;
        eventItem->setData(QVariant(NormalItem), ItemTypeRole);
        eventItem->setForeground(QBrush(textColor));
        eventItem->setData(data, Qt::DisplayRole);
        eventItem->setData(values["startDate"], SortRole);
        eventItem->setData(values["uid"], TodoModel::UIDRole);
        eventItem->setData(values["itemid"], ItemIDRole);
        eventItem->setData(values["collectionId"], CollectionRole);
        eventItem->setData(values["tooltip"], TooltipRole);
        QDateTime itemDtTime = values["startDate"].toDateTime();
        if (itemDtTime > QDateTime::currentDateTime() && QDateTime::currentDateTime().secsTo(itemDtTime) < urgency * 60) {
            eventItem->setBackground(QBrush(urgentBg));
        } else if (QDateTime::currentDateTime() > itemDtTime) {
            eventItem->setForeground(QBrush(passedFg));
        } else if (m_categoryColors.contains(category)) {
            eventItem->setBackground(QBrush(m_categoryColors.value(category)));
        }

        addItemRow(values["startDate"].toDate(), eventItem);
    }
}

void TodoModel::addTodoItem(const QMap <QString, QVariant> &values)
{
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QMap<QString, QVariant> data = values;
    QString category = values["mainCategory"].toString();

    // dont add todos starting later than a year
    if (values["hasDueDate"].toBool() == true && values["dueDate"].toDate() > QDate::currentDate().addDays(365)) {
        return;
    }

    if (values["recurs"].toBool()) {
        int c = 0;
        QList<QVariant> dtTimes = values["recurDates"].toList();
        foreach (const QVariant &eventDtTime, dtTimes) {
            if (recurringCount != 0 && c >= recurringCount)
                break;

            QStandardItem *todoItem = new QStandardItem();
            data["dueDate"] = eventDtTime;
            data["itemType"] = TodoItem;
            todoItem->setData(QVariant(TodoItem), ItemTypeRole);
            todoItem->setForeground(QBrush(textColor));
            todoItem->setData(data, Qt::DisplayRole);
            todoItem->setData(eventDtTime, SortRole);
            todoItem->setData(values["uid"], TodoModel::UIDRole);
            todoItem->setData(values["itemid"], ItemIDRole);
            todoItem->setData(values["collectionId"], CollectionRole);
            todoItem->setData(values["tooltip"], TooltipRole);
            if (values["completed"].toBool() == true) {
                todoItem->setBackground(QBrush(finishedTodoBg));
            } else if (m_categoryColors.contains(category)) {
                todoItem->setBackground(QBrush(m_categoryColors.value(category)));
            } else {
                todoItem->setBackground(QBrush(todoBg));
            }

            addItemRow(eventDtTime.toDate(), todoItem);

            ++c;
        }
    } else {
        QStandardItem *todoItem = new QStandardItem();
        data["itemType"] = TodoItem;
        todoItem->setData(QVariant(TodoItem), ItemTypeRole);
        todoItem->setForeground(QBrush(textColor));
        todoItem->setData(data, Qt::DisplayRole);
        todoItem->setData(values["dueDate"], SortRole);
        todoItem->setData(values["uid"], TodoModel::UIDRole);
        todoItem->setData(values["itemid"], ItemIDRole);
        todoItem->setData(values["collectionId"], CollectionRole);
        todoItem->setData(values["tooltip"], TooltipRole);
        if (values["completed"].toBool() == true) {
            todoItem->setBackground(QBrush(finishedTodoBg));
        } else if (m_categoryColors.contains(category)) {
            todoItem->setBackground(QBrush(m_categoryColors.value(category)));
        } else {
            todoItem->setBackground(QBrush(todoBg));
        }

        addItemRow(values["dueDate"].toDate(), todoItem);
    }
}

void TodoModel::addItemRow(QDate eventDate, QStandardItem *incidenceItem)
{
    QStandardItem *headerItem = 0;

    foreach (QStandardItem *item, m_sectionItemsMap) {
        if (eventDate < item->data(SortRole).toDate())
            break;
        else
            headerItem = item;
    }

    if (useAutoGroupHeader) {
        if ((headerItem && eventDate >= QDate::currentDate() && eventDate > headerItem->data(SortRole).toDate()) || (headerItem == 0 && eventDate > QDate::currentDate().addDays(-29))) {
            int days = QDate::currentDate().daysTo(eventDate);
            QStandardItem *item = new QStandardItem();
            initHeaderItem(item, QString("%{date}"), QString(), days);
            m_sectionItemsMap.insert(item->data(SortRole).toDate(), item);
            headerItem = item;
        }
    }


    if (headerItem) {
        headerItem->appendRow(incidenceItem);
        headerItem->sortChildren(0, Qt::AscendingOrder);
        if (headerItem->row() == -1) {
            parentItem->appendRow(headerItem);
            parentItem->sortChildren(0, Qt::AscendingOrder);
        }
        
        emit modelNeedsExpanding();
    }
}

QMap<QString, QVariant> TodoModel::eventDetails(const Akonadi::Item &item, KCalCore::Event::Ptr event)
{
    QMap <QString, QVariant> values;
    Akonadi::Collection itemCollection = m_collections.value(item.storageCollectionId());
    m_usedCollections.insert(itemCollection.name(), QString::number(itemCollection.id()));
    values["resource"] = itemCollection.resource();
    values["collectionName"] = itemCollection.name();
    values["collectionId"] = QString::number(itemCollection.id());
    values["uid"] = event->uid();
    values["itemid"] = item.id();
    values["remoteid"] = item.remoteId();
    values["summary"] = event->summary();
    values["description"] = event->description();
    values["location"] = event->location();
    QStringList categories = event->categories();
    if (categories.isEmpty()) {
        values["categories"] = i18n("Unspecified");
        values["mainCategory"] = i18n("Unspecified");
    } else {
        values["categories"] = categories;
        values["mainCategory"] = categories.first();
    }

    values["status"] = event->status();
    values["startDate"] = event->dtStart().toLocalZone().dateTime();
    values["endDate"] = event->dtEnd().toLocalZone().dateTime();

    bool recurs = event->recurs();
    values["recurs"] = recurs;
    QList<QVariant> recurDates;
    if (recurs) {
        KCalCore::Recurrence *r = event->recurrence();
        KCalCore::DateTimeList dtTimes = r->timesInInterval(KDateTime(QDate::currentDate()), KDateTime(QDate::currentDate()).addDays(365));
        dtTimes.sortUnique();
        foreach (const KDateTime &t, dtTimes) {
            recurDates << QVariant(t.toLocalZone().dateTime());
        }
    }
    values["recurDates"] = recurDates;

    if (event->customProperty("KABC", "BIRTHDAY") == QString("YES") || categories.contains(i18n("Birthday")) || categories.contains("Birthday")) {
        values["isBirthday"] = QVariant(true);
    } else {
        values["isBirthday"] = QVariant(false);
    }

    event->customProperty("KABC", "ANNIVERSARY") == QString("YES") ? values ["isAnniversary"] = QVariant(true) : QVariant(false);
    values["contactName"] = event->customProperty("KABC", "NAME-1");
    values["tooltip"] = KCalUtils::IncidenceFormatter::toolTipStr(itemCollection.name(), event, event->dtStart().date(), true, KDateTime::Spec::LocalZone());

    return values;
}

QMap<QString, QVariant> TodoModel::todoDetails(const Akonadi::Item &item, KCalCore::Todo::Ptr todo)
{
    QMap <QString, QVariant> values;
    Akonadi::Collection itemCollection = m_collections.value(item.storageCollectionId());
    m_usedCollections.insert(itemCollection.name(), QString::number(itemCollection.id()));
    values["resource"] = itemCollection.resource();
    values["collectionName"] = itemCollection.name();
    values["collectionId"] = QString::number(itemCollection.id());
    values["uid"] = todo->uid();
    values["itemid"] = item.id();
    values["remoteid"] = item.remoteId();
    values["summary"] = todo->summary();
    values["description"] = todo->description();
    values["location"] = todo->location();
    QStringList categories = todo->categories();
    if (categories.isEmpty()) {
        values["categories"] = i18n("Unspecified");
        values["mainCategory"] = i18n("Unspecified");
    } else {
        values["categories"] = categories;
        values["mainCategory"] = categories.first();
    }

    values["completed"] = todo->isCompleted();
    values["percent"] = todo->percentComplete();
    if (todo->hasStartDate()) {
        values["startDate"] = todo->dtStart(false).toLocalZone().dateTime();
        values["hasStartDate"] = true;
    } else {
        values["startDate"] = QDateTime();
        values["hasStartDate"] = false;
    }
    values["completedDate"] = todo->completed().toLocalZone().dateTime();
    values["inProgress"] = todo->isInProgress(false);
    values["isOverdue"] = todo->isOverdue();
    if (todo->hasDueDate()) {
        values["dueDate"] = todo->dtDue().toLocalZone().dateTime();
        values["hasDueDate"] = true;
    } else {
        values["dueDate"] = QDateTime::currentDateTime().addDays(366);
        values["hasDueDate"] = false;
    }

    bool recurs = todo->recurs();
    values["recurs"] = recurs;
    QList<QVariant> recurDates;
    if (recurs) {
        KCalCore::Recurrence *r = todo->recurrence();
        KCalCore::DateTimeList dtTimes = r->timesInInterval(KDateTime(QDate::currentDate()), KDateTime(QDate::currentDate()).addDays(365));
        dtTimes.sortUnique();
        foreach (const KDateTime &t, dtTimes) {
            recurDates << QVariant(t.toLocalZone().dateTime());
        }
    }
    values["recurDates"] = recurDates;

    values["tooltip"] = KCalUtils::IncidenceFormatter::toolTipStr(itemCollection.name(), todo, todo->dtStart().date(), true, KDateTime::Spec::LocalZone());

    return values;
}

QMap<QString, QString> TodoModel::usedCollections()
{
    return m_usedCollections;
}

#include "todomodel.moc"
