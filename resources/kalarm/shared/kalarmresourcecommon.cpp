/*
 *  kalarmresourcecommon.cpp  -  common functions for KAlarm Akonadi resources
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2009-2019 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kalarmresourcecommon.h"

#include <kalarmcal/compatibilityattribute.h>
#include <kalarmcal/eventattribute.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionModifyJob>

#include <KCalendarCore/FileStorage>
#include <KCalendarCore/MemoryCalendar>

#include <KLocalizedString>

#include <QDebug>
#include <QTime>

using namespace Akonadi;
using namespace KCalendarCore;
using namespace KAlarmCal;

class Private : public QObject
{
    Q_OBJECT
public:
    Private(QObject *parent)
        : QObject(parent)
    {
    }

    static Private *mInstance;

private Q_SLOTS:
    void modifyCollectionJobDone(KJob *);
};
Private *Private::mInstance = nullptr;

namespace KAlarmResourceCommon
{
/******************************************************************************
 * Perform common initialisation for KAlarm resources.
 */
void initialise(QObject *parent)
{
    // Create an object which can receive signals.
    if (!Private::mInstance) {
        Private::mInstance = new Private(parent);
    }

    // Set a default start-of-day time for date-only alarms.
    KAEvent::setStartOfDay(QTime(0, 0, 0));

    AttributeFactory::registerAttribute<CompatibilityAttribute>();
    AttributeFactory::registerAttribute<EventAttribute>();
}

/******************************************************************************
 * Find the compatibility of an existing calendar file, and convert it in
 * memory to the current KAlarm format (if possible).
 */
KACalendar::Compat getCompatibility(const FileStorage::Ptr &fileStorage, int &version)
{
    QString versionString;
    version = KACalendar::updateVersion(fileStorage, versionString);
    switch (version) {
    case KACalendar::IncompatibleFormat:
        return KACalendar::Incompatible; // calendar is not in KAlarm format, or is in a future format
    case KACalendar::CurrentFormat:
        return KACalendar::Current; // calendar is in the current format
    default:
        return KACalendar::Convertible; // calendar is in an out of date format
    }
}

/******************************************************************************
 * Set an event into a new item's payload and return the new item.
 * The caller should signal its retrieval by calling itemRetrieved(newitem).
 * NOTE: the caller must set the event's compatibility beforehand.
 */
Item retrieveItem(const Akonadi::Item &item, KAEvent &event)
{
    const QString mime = CalEvent::mimeType(event.category());
    if (item.hasAttribute<EventAttribute>()) {
        event.setCommandError(item.attribute<EventAttribute>()->commandError());
    }

    Item newItem = item;
    newItem.setMimeType(mime);
    newItem.setPayload<KAEvent>(event);
    return newItem;
}

/******************************************************************************
 * Called when an item has been changed to validate it.
 * Reply = the KAEvent for the item
 *       = invalid if error, in which case errorMsg contains the error message
 *         (which will be empty if the KAEvent is simply invalid).
 */
KAEvent checkItemChanged(const Akonadi::Item &item, QString &errorMsg)
{
    KAEvent event;
    if (item.hasPayload<KAEvent>()) {
        event = item.payload<KAEvent>();
    }
    if (event.isValid()) {
        if (item.remoteId() != event.id()) {
            qWarning() << "Item ID" << item.remoteId() << "differs from payload ID" << event.id();
            errorMsg = i18nc("@info", "Item ID %1 differs from payload ID %2.", item.remoteId(), event.id());
            return KAEvent();
        }
    }

    errorMsg.clear();
    return event;
}

/******************************************************************************
 * Set a collection's compatibility attribute.
 * Note that because this parameter is set asynchronously by the resource, it
 * can't be stored in the same attribute as other collection parameters which
 * are written by the application. This avoids the resource and application
 * overwriting each other's changes if they attempt simultaneous updates.
 */
void setCollectionCompatibility(const Collection &collection, KACalendar::Compat compatibility, int version)
{
    qDebug() << "KAlarmResourceCommon::setCollectionCompatibility:" << collection.id() << "->" << compatibility << version;
    // Update the CompatibilityAttribute value only.
    // Note that we can't supply 'collection' to CollectionModifyJob since
    // that may also contain the CollectionAttribute value, which is read-only
    // for the resource. So create a new Collection instance and only set a
    // value for CompatibilityAttribute.
    Collection col(collection.id());
    if (!collection.isValid()) {
        col.setParentCollection(collection.parentCollection());
        col.setRemoteId(collection.remoteId());
    }
    auto attr = col.attribute<CompatibilityAttribute>(Collection::AddIfMissing);
    attr->setCompatibility(compatibility);
    attr->setVersion(version);
    Q_ASSERT(Private::mInstance);
    auto job = new CollectionModifyJob(col, Private::mInstance->parent());
    Private::mInstance->connect(job, SIGNAL(result(KJob *)), SLOT(modifyCollectionJobDone(KJob *)));
}

/******************************************************************************
 * Return an error message common to more than one resource.
 */
QString errorMessage(ErrorCode code, const QString &param)
{
    switch (code) {
    case UidNotFound:
        return i18nc("@info", "Event with uid '%1' not found.", param);
    case NotCurrentFormat:
        return i18nc("@info", "Calendar is not in current KAlarm format.");
    case EventNotCurrentFormat:
        return i18nc("@info", "Event with uid '%1' is not in current KAlarm format.", param);
    case EventNoAlarms:
        return i18nc("@info", "Event with uid '%1' contains no usable alarms.", param);
    case EventReadOnly:
        return i18nc("@info", "Event with uid '%1' is read only", param);
    case CalendarAdd:
        return i18nc("@info", "Failed to add event with uid '%1' to calendar", param);
    }
    return QString();
}
} // namespace KAlarmResourceCommon

/******************************************************************************
 * Called when a collection modification job has completed, to report any error.
 */
void Private::modifyCollectionJobDone(KJob *j)
{
    qDebug() << "KAlarmResourceCommon::modifyCollectionJobDone";
    if (j->error()) {
        Collection collection = static_cast<CollectionModifyJob *>(j)->collection();
        qCritical() << "Error: modifyCollectionJobDone: collection" << collection.id() << ":" << j->errorString();
    }
}

#include "kalarmresourcecommon.moc"
