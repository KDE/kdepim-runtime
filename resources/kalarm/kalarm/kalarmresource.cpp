/*
 *  kalarmresource.cpp  -  Akonadi resource for KAlarm
 *  Program:  kalarm
 *  Copyright © 2009-2019 David Jarvie <djarvie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "kalarmresource.h"
#include "kalarmresourcecommon.h"

#include <kalarmcal/compatibilityattribute.h>
#include <kalarmcal/kacalendar.h>
#include <kalarmcal/kaevent.h>

#include <attributefactory.h>
#include <collectionfetchjob.h>
#include <collectionfetchscope.h>
#include <collectionmodifyjob.h>

#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Incidence>

#include "kalarmresource_debug.h"

using namespace Akonadi;
using namespace Akonadi_KAlarm_Resource;
using namespace KAlarmCal;
using KAlarmResourceCommon::errorMessage;

KAlarmResource::KAlarmResource(const QString &id)
    : ICalResourceBase(id)
    , mCompatibility(KACalendar::Incompatible)
    , mFileCompatibility(KACalendar::Incompatible)
    , mVersion(KACalendar::MixedFormat)
    , mFileVersion(KACalendar::IncompatibleFormat)
{
    qCDebug(KALARMRESOURCE_LOG) << id << "Starting";
    KAlarmResourceCommon::initialise(this);
    initialise(mSettings->alarmTypes(), QStringLiteral("kalarm"));
    connect(mSettings, &Settings::configChanged, this, &KAlarmResource::settingsChanged);

    // Start a job to fetch the collection attributes
    fetchCollection(SLOT(collectionFetchResult(KJob*)));
}

KAlarmResource::~KAlarmResource()
{
}

/******************************************************************************
* Reimplemented to fetch collection attributes after creating the collection.
*/
void KAlarmResource::retrieveCollections()
{
    qCDebug(KALARMRESOURCE_LOG) << identifier() << "retrieveCollections";
    mSettings->load();
    mSupportedMimetypes = mSettings->alarmTypes();
    if (mSettings->path().isEmpty()) {
        // Don't configure the collection when no calendar file is defined.
        // Otherwise, multiple collections may be created for the resource.
        collectionsRetrieved({});
    } else {
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "retrieveCollections:" << mSettings->displayName() << mSettings->path();
        if (!mSettings->displayName().isEmpty()) {
            setName(mSettings->displayName());
        }
        ICalResourceBase::retrieveCollections();
        fetchCollection(SLOT(collectionFetchResult(KJob*)));
    }
}

/******************************************************************************
* Called when the collection fetch job completes.
* Check the calendar file's compatibility status if pending.
*/
void KAlarmResource::collectionFetchResult(KJob *j)
{
    if (j->error()) {
        // An error occurred. Note that if it's a new resource, it will complain
        // about an invalid collection if the collection has not yet been created.
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "Error: collectionFetchResult:" << j->errorString();
    } else {
        bool firstTime = !mFetchedAttributes;
        mFetchedAttributes = true;
        CollectionFetchJob *job = static_cast<CollectionFetchJob *>(j);
        const Collection::List collections = job->collections();
        if (collections.isEmpty()) {
            qCDebug(KALARMRESOURCE_LOG) << identifier() << "collectionFetchResult: resource's collection not found";
        } else {
            if (collections.count() > 1) {
                qCCritical(KALARMRESOURCE_LOG) << identifier() << "Error! Resource contains multiple collections!";
            }

            // Check whether calendar file format needs to be updated
            const Collection &c(collections[0]);
            qCDebug(KALARMRESOURCE_LOG) << identifier() << "collectionFetchResult: Fetched collection" << c.id();
            if (firstTime && mSettings->path().isEmpty()) {
                // Initialising a resource which seems to have no stored
                // settings config file. Recreate the settings.
                static const Collection::Rights writableRights = Collection::CanChangeItem | Collection::CanCreateItem | Collection::CanDeleteItem;
                qCDebug(KALARMRESOURCE_LOG) << identifier() << "collectionFetchResult: Recreating config for remote id:" << c.remoteId();
                mSettings->setPath(c.remoteId());
                mSettings->setDisplayName(c.name());
                mSettings->setAlarmTypes(c.contentMimeTypes());
                mSettings->setReadOnly((c.rights() & writableRights) != writableRights);
                mSettings->save();
                synchronize();   // tell the server to use the new config
            }

            // Read the file if it already exists. If it's a new file, this will
            // trigger it to be written later.
            if (!mHaveReadFile) {
                readFile();
            }

            checkFileCompatibility(c, true);
        }
    }
}

/******************************************************************************
* Reimplemented to read data from the given file.
* This is called every time the resource starts up (see SingleFileResourceBase
* constructor).
* Find the calendar file's compatibility with the current KAlarm format.
* The file is always local; loading from the network is done automatically if
* needed.
*/
bool KAlarmResource::readFromFile(const QString &fileName)
{
    qCDebug(KALARMRESOURCE_LOG) << identifier() << "readFromFile:" << fileName;
//TODO Notify user if error occurs on next line
    if (!ICalResourceBase::readFromFile(fileName)) {
        return false;
    }
    if (calendar()->incidences().isEmpty()) {
        // It's a new file. Set up the KAlarm custom property.
        KACalendar::setKAlarmVersion(calendar());
    }
    mFileCompatibility = KAlarmResourceCommon::getCompatibility(fileStorage(), mFileVersion);
    mHaveReadFile = true;

    if (mFetchedAttributes) {
        // The old calendar file version and compatibility have been read from
        // the database. Check if the file format needs to be converted.
        checkFileCompatibility();
    }
    return true;
}

/******************************************************************************
* To be called when the collection attributes have been fetched, or if they
* have changed.
* Check if the recorded calendar version and compatibility are different from
* the actual backend file, and if necessary convert the calendar in memory to
* the current version.
* If 'createAttribute' is true, the CompatibilityAttribute will be created if
* it does not already exist.
*/
void KAlarmResource::checkFileCompatibility(const Collection &collection, bool createAttribute)
{
    if (collection.isValid()
        && collection.hasAttribute<CompatibilityAttribute>()) {
        // Update our note of the calendar version and compatibility
        const CompatibilityAttribute *attr = collection.attribute<CompatibilityAttribute>();
        mCompatibility = attr->compatibility();
        mVersion = attr->version();
        createAttribute = false;
    }
    if (mHaveReadFile
        && (createAttribute
            || mFileCompatibility != mCompatibility || mFileVersion != mVersion)) {
        // The actual file's version and compatibility are different from
        // those in the Akonadi database, so update the database attributes.
        mCompatibility = mFileCompatibility;
        mVersion = mFileVersion;
        const Collection c(collection);
        if (c.isValid()) {
            KAlarmResourceCommon::setCollectionCompatibility(c, mCompatibility, mVersion);
        } else {
            fetchCollection(SLOT(setCompatibility(KJob*)));
        }
    }
}

/******************************************************************************
* Called when a collection fetch job completes.
* Set the compatibility attribute for the collection.
*/
void KAlarmResource::setCompatibility(KJob *j)
{
    CollectionFetchJob *job = static_cast<CollectionFetchJob *>(j);
    if (j->error()) {
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "Error: setCompatibility:" << j->errorString();
    } else if (job->collections().isEmpty()) {
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "Error: setCompatibility: resource's collection not found";
    } else {
        KAlarmResourceCommon::setCollectionCompatibility(job->collections().at(0), mCompatibility, mVersion);
    }
}

/******************************************************************************
* Reimplemented to write data to the given file.
* The file is always local.
*/
bool KAlarmResource::writeToFile(const QString &fileName)
{
    qCDebug(KALARMRESOURCE_LOG) << identifier() << "writeToFile:" << fileName;
    if (mCompatibility != KACalendar::Current && !mUpdatingFormat) {
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "Error: writeToFile: wrong format";
        return false;
    }
    if (calendar() && calendar()->incidences().isEmpty()) {
        // It's an empty file. Set up the KAlarm custom property.
        KACalendar::setKAlarmVersion(calendar());
    }
    return ICalResourceBase::writeToFile(fileName);
}

/******************************************************************************
* Retrieve an event from the calendar, whose uid and Akonadi id are given by
* 'item' (item.remoteId() and item.id() respectively).
* Set the event into a new item's payload, and signal its retrieval by calling
* itemRetrieved(newitem).
*/
bool KAlarmResource::doRetrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts);
    const QString rid = item.remoteId();
    const KCalendarCore::Event::Ptr kcalEvent = calendar()->event(rid);
    if (!kcalEvent) {
        qCWarning(KALARMRESOURCE_LOG) << identifier() << "doRetrieveItem: Event not found:" << rid;
        Q_EMIT error(errorMessage(KAlarmResourceCommon::UidNotFound, rid));
        return false;
    }

    if (kcalEvent->alarms().isEmpty()) {
        qCWarning(KALARMRESOURCE_LOG) << identifier() << "doRetrieveItem: KCalendarCore::Event has no alarms:" << rid;
        Q_EMIT error(errorMessage(KAlarmResourceCommon::EventNoAlarms, rid));
        return false;
    }

    KAEvent event(kcalEvent);
    const QString mime = CalEvent::mimeType(event.category());
    if (mime.isEmpty()) {
        qCWarning(KALARMRESOURCE_LOG) << identifier() << "doRetrieveItem: KAEvent has no alarms:" << rid;
        Q_EMIT error(errorMessage(KAlarmResourceCommon::EventNoAlarms, rid));
        return false;
    }
    event.setCompatibility(mCompatibility);
    const Item newItem = KAlarmResourceCommon::retrieveItem(item, event);
    itemRetrieved(newItem);
    return true;
}

/******************************************************************************
* Called when the resource settings have changed.
* Update the supported mime types if the AlarmTypes setting has changed.
* Update the storage format if UpdateStorageFormat setting = true.
*/
void KAlarmResource::settingsChanged()
{
    qCDebug(KALARMRESOURCE_LOG) << identifier() << "settingsChanged";
    const QStringList mimeTypes = mSettings->alarmTypes();
    if (mimeTypes != mSupportedMimetypes) {
        mSupportedMimetypes = mimeTypes;
    }

    if (mSettings->updateStorageFormat()) {
        // This is a flag to request that the backend calendar storage format should
        // be updated to the current KAlarm format.
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "Update storage format";
        fetchCollection(SLOT(updateFormat(KJob*)));
    }
}

/******************************************************************************
* Called when a collection fetch job completes.
* Update the backend calendar storage format to the current KAlarm format.
*/
void KAlarmResource::updateFormat(KJob *j)
{
    CollectionFetchJob *job = static_cast<CollectionFetchJob *>(j);
    if (j->error()) {
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "Error: updateFormat:" << j->errorString();
    } else if (job->collections().isEmpty()) {
        qCDebug(KALARMRESOURCE_LOG) << identifier() << "Error: updateFormat: resource's collection not found";
    } else {
        const Collection c(job->collections().at(0));
        if (c.hasAttribute<CompatibilityAttribute>()) {
            const CompatibilityAttribute *attr = c.attribute<CompatibilityAttribute>();
            if (attr->compatibility() != mCompatibility) {
                qCDebug(KALARMRESOURCE_LOG) << identifier() << "updateFormat: Compatibility changed:" << mCompatibility << "->" << attr->compatibility();
            }
        }
        switch (mCompatibility) {
        case KACalendar::Current:
            qCWarning(KALARMRESOURCE_LOG) << identifier() << "updateFormat: Already current storage format";
            break;
        case KACalendar::Incompatible:
        default:
            qCWarning(KALARMRESOURCE_LOG) << identifier() << "updateFormat: Incompatible storage format: compat=" << mCompatibility;
            break;
        case KACalendar::Converted:
        case KACalendar::Convertible:
        {
            if (mSettings->readOnly()) {
                qCWarning(KALARMRESOURCE_LOG) << identifier() << "updateFormat: Cannot update storage format for a read-only resource";
                break;
            }
            // Update the backend storage format to the current KAlarm format
            const QString filename = fileStorage()->fileName();
            qCDebug(KALARMRESOURCE_LOG) << identifier() << "updateFormat: Updating storage for" << filename;
            KACalendar::setKAlarmVersion(fileStorage()->calendar());
            mUpdatingFormat = true;
            bool ok = writeToFile(filename);
            mUpdatingFormat = false;
            if (!ok) {
                qCWarning(KALARMRESOURCE_LOG) << identifier() << "updateFormat: Error updating calendar storage format";
                break;
            }
            // Prevent a new file read being triggered by writeToFile(), which
            // would replace the current Collection by a new one.
            mCurrentHash = calculateHash(filename);

            mCompatibility = mFileCompatibility = KACalendar::Current;
            mVersion = mFileVersion = KACalendar::CurrentFormat;
            KAlarmResourceCommon::setCollectionCompatibility(c, mCompatibility, 0);
            break;
        }
        }
        mSettings->setUpdateStorageFormat(false);
        mSettings->save();
    }
}

/******************************************************************************
* Called when an item has been added to the collection.
* Store the event in the calendar, and set its Akonadi remote ID to the
* KAEvent's UID.
*/
void KAlarmResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &)
{
    if (!checkItemAddedChanged<KAEvent>(item, CheckForAdded)) {
        return;
    }
    if (mCompatibility != KACalendar::Current) {
        qCWarning(KALARMRESOURCE_LOG) << identifier() << "itemAdded: Calendar not in current format";
        cancelTask(errorMessage(KAlarmResourceCommon::NotCurrentFormat));
        return;
    }
    const KAEvent event = item.payload<KAEvent>();
    KCalendarCore::Event::Ptr kcalEvent(new KCalendarCore::Event);
    event.updateKCalEvent(kcalEvent, KAEvent::UID_SET);
    if (!calendar()->addIncidence(kcalEvent)) {
        qCritical() << "Error adding event with id" << event.id() << ", item id" << item.id();
        cancelTask(errorMessage(KAlarmResourceCommon::CalendarAdd, event.id()));
        return;
    }

    Item it(item);
    it.setRemoteId(kcalEvent->uid());
    scheduleWrite();
    changeCommitted(it);
}

/******************************************************************************
* Called when an item has been changed.
* Store the changed event in the calendar, and delete the original event.
*/
void KAlarmResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)
    if (!checkItemAddedChanged<KAEvent>(item, CheckForChanged)) {
        return;
    }
    QString errorMsg;
    if (mCompatibility != KACalendar::Current) {
        qCWarning(KALARMRESOURCE_LOG) << identifier() << "itemChanged: Calendar not in current format";
        cancelTask(errorMessage(KAlarmResourceCommon::NotCurrentFormat));
        return;
    }
    const KAEvent event = KAlarmResourceCommon::checkItemChanged(item, errorMsg);
    if (!event.isValid()) {
        if (errorMsg.isEmpty()) {
            changeProcessed();
        } else {
            cancelTask(errorMsg);
        }
        return;
    }

    KCalendarCore::Incidence::Ptr incidence = calendar()->incidence(item.remoteId());
    if (incidence) {
        if (incidence->isReadOnly()) {
            qCWarning(KALARMRESOURCE_LOG) << identifier() << "itemChanged: Event is read only:" << event.id();
            cancelTask(errorMessage(KAlarmResourceCommon::EventReadOnly, event.id()));
            return;
        }
        if (incidence->type() == KCalendarCore::Incidence::TypeEvent) {
            calendar()->deleteIncidence(incidence);   // it's not an Event
            incidence.clear();
        } else {
            KCalendarCore::Event::Ptr ev(incidence.staticCast<KCalendarCore::Event>());
            event.updateKCalEvent(ev, KAEvent::UID_SET);
            calendar()->setModified(true);
        }
    }
    if (!incidence) {
        // not in the calendar yet, should not happen -> add it
        KCalendarCore::Event::Ptr kcalEvent(new KCalendarCore::Event);
        event.updateKCalEvent(kcalEvent, KAEvent::UID_SET);
        calendar()->addIncidence(kcalEvent);
    }
    scheduleWrite();
    changeCommitted(item);
}

/******************************************************************************
* Called when a collection has been changed.
* Determine the calendar file's storage format.
*/
void KAlarmResource::collectionChanged(const Akonadi::Collection &collection)
{
    qCDebug(KALARMRESOURCE_LOG) << identifier() << "collectionChanged";
    ICalResourceBase::collectionChanged(collection);

    mFetchedAttributes = true;
    // Check whether calendar file format needs to be updated
    checkFileCompatibility(collection);
}

/******************************************************************************
* Retrieve all events from the calendar, and set each into a new item's
* payload. Items are identified by their remote IDs. The Akonadi ID is not
* used.
* Signal the retrieval of the items by calling itemsRetrieved(items), which
* updates Akonadi with any changes to the items. itemsRetrieved() compares
* the new and old items, matching them on the remoteId(). If the flags or
* payload have changed, or the Item has any new Attributes, the Akonadi
* storage is updated.
*/
void KAlarmResource::doRetrieveItems(const Akonadi::Collection &collection)
{
    qCDebug(KALARMRESOURCE_LOG) << identifier() << "doRetrieveItems";

    // Set the collection's compatibility status
    KAlarmResourceCommon::setCollectionCompatibility(collection, mCompatibility, mVersion);

    // Retrieve events from the calendar
    const KCalendarCore::Event::List events = calendar()->events();
    Item::List items;
    for (const KCalendarCore::Event::Ptr &kcalEvent : qAsConst(events)) {
        if (kcalEvent->alarms().isEmpty()) {
            qCWarning(KALARMRESOURCE_LOG) << identifier() << "doRetrieveItems: KCalendarCore::Event has no alarms:" << kcalEvent->uid();
            continue;    // ignore events without alarms
        }

        const KAEvent event(kcalEvent);
        const QString mime = CalEvent::mimeType(event.category());
        if (mime.isEmpty()) {
            qCWarning(KALARMRESOURCE_LOG) << identifier() << "doRetrieveItems: KAEvent has no alarms:" << event.id();
            continue;   // event has no usable alarms
        }

        Item item(mime);
        item.setRemoteId(kcalEvent->uid());
        item.setPayload(event);
        items << item;
    }
    itemsRetrieved(items);
}

/******************************************************************************
* Execute a CollectionFetchJob to fetch details of this resource's collection.
*/
CollectionFetchJob *KAlarmResource::fetchCollection(const char *slot)
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel);
    job->fetchScope().setResource(identifier());
    connect(job, SIGNAL(result(KJob*)), slot);
    return job;
}

AKONADI_RESOURCE_MAIN(KAlarmResource)
