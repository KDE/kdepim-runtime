/*
 *  kalarmresource.cpp  -  Akonadi resource for KAlarm
 *  Program:  kalarm
 *  Copyright © 2009-2012 by David Jarvie <djarvie@kde.org>
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
#include "alarmtyperadiowidget.h"

#include <kalarmcal/compatibilityattribute.h>
#include <kalarmcal/kacalendar.h>
#include <kalarmcal/kaevent.h>

#include <akonadi/agentfactory.h>
#include <akonadi/attributefactory.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionmodifyjob.h>

#include <kcalcore/memorycalendar.h>
#include <kcalcore/incidence.h>

#include <klocale.h>
#include <kdebug.h>

using namespace Akonadi;
using namespace Akonadi_KAlarm_Resource;
using namespace KAlarmCal;
using KAlarmResourceCommon::errorMessage;


KAlarmResource::KAlarmResource(const QString& id)
    : ICalResourceBase(id),
      mCompatibility(KACalendar::Incompatible),
      mFileCompatibility(KACalendar::Incompatible),
      mVersion(KACalendar::MixedFormat),
      mFileVersion(KACalendar::IncompatibleFormat),
      mHaveReadFile(false),
      mFetchedAttributes(false)
{
    kDebug() << id;
    KAlarmResourceCommon::initialise(this);
    initialise(mSettings->alarmTypes(), "kalarm");
    connect(mSettings, SIGNAL(configChanged()), SLOT(settingsChanged()));

    // Fetch the collection attributes
    CollectionFetchJob* job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel);
    job->fetchScope().setResource(identifier());
    connect(job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)));
}

KAlarmResource::~KAlarmResource()
{
}

/******************************************************************************
* Customize the configuration dialog before it is displayed.
*/
void KAlarmResource::customizeConfigDialog(SingleFileResourceConfigDialog<Settings>* dlg)
{
    ICalResourceBase::customizeConfigDialog(dlg);
#ifdef KDEPIM_MOBILE_UI
    dlg->setFilter("*.ics");
#endif
    mTypeSelector = new AlarmTypeRadioWidget(dlg);
    QStringList types = mSettings->alarmTypes();
    CalEvent::Type alarmType = CalEvent::ACTIVE;
    if (!types.isEmpty())
        alarmType = CalEvent::type(types[0]);
    mTypeSelector->setAlarmType(alarmType);
    dlg->appendWidget(mTypeSelector);
    dlg->setMonitorEnabled(false);
    QString title;
    switch (alarmType)
    {
        case CalEvent::ACTIVE:
            title = i18nc("@title:window", "Select Active Alarm Calendar");
            break;
        case CalEvent::ARCHIVED:
            title = i18nc("@title:window", "Select Archived Alarm Calendar");
            break;
        case CalEvent::TEMPLATE:
            title = i18nc("@title:window", "Select Alarm Template Calendar");
            break;
        default:
            return;
    }
    dlg->setCaption(title);
}

/******************************************************************************
* Save extra settings after the configuration dialog has been accepted.
*/
void KAlarmResource::configDialogAcceptedActions(SingleFileResourceConfigDialog<Settings>*)
{
    mSettings->setAlarmTypes(CalEvent::mimeTypes(mTypeSelector->alarmType()));
    mSettings->writeConfig();
}

/******************************************************************************
* Reimplemented to fetch collection attributes after creating the collection.
*/
void KAlarmResource::retrieveCollections()
{
    kDebug();
    mSupportedMimetypes = mSettings->alarmTypes();
    ICalResourceBase::retrieveCollections();

    Collection c;
    c.setParentCollection(Collection::root());
    c.setRemoteId(remoteIdFromPath());
    CollectionFetchJob* job = new CollectionFetchJob(c, CollectionFetchJob::FirstLevel);
    connect(job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)));
}

/******************************************************************************
* Called when the collection fetch job completes.
* Check the calendar file's compatibility status if pending.
*/
void KAlarmResource::collectionFetchResult(KJob* j)
{
    if (j->error())
    {
        // An error occurred. Note that if it's a new resource, it will complain
        // about an invalid collection if the collection has not yet been created.
        kDebug() << "Error: " << j->errorString();
    }
    else
    {
        mFetchedAttributes = true;
        CollectionFetchJob* job = static_cast<CollectionFetchJob*>(j);
        Collection::List collections = job->collections();
        if (!collections.isEmpty())
        {
            // Check whether calendar file format needs to be updated
            kDebug() << "Fetched collection";
            checkFileCompatibility(collections[0]);
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
bool KAlarmResource::readFromFile(const QString& fileName)
{
    kDebug() << fileName;
#ifdef __GNUC__
#warning Notify user if error occurs on next line
#endif
    if (!ICalResourceBase::readFromFile(fileName))
        return false;
    if (calendar()->incidences().isEmpty())
    {
        // It's a new file. Set up the KAlarm custom property.
        KACalendar::setKAlarmVersion(calendar());
    }
    mFileCompatibility = KAlarmResourceCommon::getCompatibility(fileStorage(), mFileVersion);
    mHaveReadFile = true;

    if (mFetchedAttributes)
    {
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
*/
void KAlarmResource::checkFileCompatibility(const Collection& collection)
{
    if (collection.isValid()
    &&  collection.hasAttribute<CompatibilityAttribute>())
    {
        // Update our note of the calendar version and compatibility
        CompatibilityAttribute* attr = collection.attribute<CompatibilityAttribute>();
        mCompatibility = attr->compatibility();
        mVersion       = attr->version();
    }
    if (mHaveReadFile
    &&  (mFileCompatibility != mCompatibility  ||  mFileVersion != mVersion))
    {
        // The actual file's version and compatibility are different from
        // those in the Akonadi database, so update the database attributes.
        mCompatibility = mFileCompatibility;
        mVersion       = mFileVersion;
        Collection c(collection);
        if (!c.isValid())
        {
            c.setParentCollection(Collection::root());
            c.setRemoteId(remoteIdFromPath());
        }
        KAlarmResourceCommon::setCollectionCompatibility(c, mCompatibility, mVersion);
    }
}

/******************************************************************************
* Reimplemented to write data to the given file.
* The file is always local.
*/
bool KAlarmResource::writeToFile(const QString& fileName)
{
    kDebug() << fileName;
    if (calendar()->incidences().isEmpty())
    {
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
bool KAlarmResource::doRetrieveItem(const Akonadi::Item& item, const QSet<QByteArray>& parts)
{
    Q_UNUSED(parts);
    const QString rid = item.remoteId();
    const KCalCore::Event::Ptr kcalEvent = calendar()->event(rid);
    if (!kcalEvent)
    {
        kWarning() << "Event not found:" << rid;
        emit error(errorMessage(KAlarmResourceCommon::UidNotFound, rid));
        return false;
    }

    if (kcalEvent->alarms().isEmpty())
    {
        kWarning() << "KCalCore::Event has no alarms:" << rid;
        emit error(errorMessage(KAlarmResourceCommon::EventNoAlarms, rid));
        return false;
    }

    KAEvent event(kcalEvent);
    QString mime = CalEvent::mimeType(event.category());
    if (mime.isEmpty())
    {
        kWarning() << "KAEvent has no alarms:" << rid;
        emit error(errorMessage(KAlarmResourceCommon::EventNoAlarms, rid));
        return false;
    }
    event.setCompatibility(mCompatibility);
    Item newItem = KAlarmResourceCommon::retrieveItem(item, event);
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
    kDebug();
    QStringList mimeTypes = mSettings->alarmTypes();
    if (mimeTypes != mSupportedMimetypes)
        mSupportedMimetypes = mimeTypes;

    if (mSettings->updateStorageFormat())
    {
        // This is a flag to request that the backend calendar storage format should
        // be updated to the current KAlarm format.
        kDebug() << "Update storage format";
        Collection c;
        c.setParentCollection(Collection::root());
        c.setRemoteId(remoteIdFromPath());
        if (c.hasAttribute<CompatibilityAttribute>())
        {
            CompatibilityAttribute* attr = c.attribute<CompatibilityAttribute>();
            if (attr->compatibility() != mCompatibility)
                kDebug()<<"Compatibility changed:"<<mCompatibility<<"->"<<attr->compatibility();
        }
        switch (mCompatibility)
        {
            case KACalendar::Current:
                kWarning() << "Already current storage format";
                break;
            case KACalendar::Incompatible:
            default:
                kWarning() << "Incompatible storage format: compat=" << mCompatibility;
                break;
            case KACalendar::Converted:
            case KACalendar::Convertible:
            {
                if (mSettings->readOnly())
                {
                    kWarning() << "Cannot update storage format for a read-only resource";
                    break;
                }
                // Update the backend storage format to the current KAlarm format
                QString filename = fileStorage()->fileName();
                kDebug() << "Updating storage for" << filename;
                KACalendar::setKAlarmVersion(fileStorage()->calendar());
                if (!writeToFile(filename))
                {
                    kWarning() << "Error updating calendar storage format";
                    break;
                }
                // Prevent a new file read being triggered by writeToFile(), which
                // would replace the current Collection by a new one.
                mCurrentHash = calculateHash(filename);

                mCompatibility = mFileCompatibility = KACalendar::Current;
                mVersion = mFileVersion = KACalendar::CurrentFormat;
                Collection c;
                c.setParentCollection(Collection::root());
                c.setRemoteId(remoteIdFromPath());
                KAlarmResourceCommon::setCollectionCompatibility(c, mCompatibility, 0);
                break;
            }
        }
        mSettings->setUpdateStorageFormat(false);
        mSettings->writeConfig();
    }
}

/******************************************************************************
* Called when an item has been added to the collection.
* Store the event in the calendar, and set its Akonadi remote ID to the
* KAEvent's UID.
*/
void KAlarmResource::itemAdded(const Akonadi::Item& item, const Akonadi::Collection&)
{
    if (!checkItemAddedChanged<KAEvent>(item, CheckForAdded))
        return;
    if (mCompatibility != KACalendar::Current)
    {
        kWarning() << "Calendar not in current format";
        cancelTask(errorMessage(KAlarmResourceCommon::NotCurrentFormat));
        return;
    }
    KAEvent event = item.payload<KAEvent>();
    KCalCore::Event::Ptr kcalEvent(new KCalCore::Event);
    event.updateKCalEvent(kcalEvent, KAEvent::UID_SET);
    calendar()->addIncidence(kcalEvent);

    Item it(item);
    it.setRemoteId(kcalEvent->uid());
    scheduleWrite();
    changeCommitted(it);
}

/******************************************************************************
* Called when an item has been changed.
* Store the changed event in the calendar, and delete the original event.
*/
void KAlarmResource::itemChanged(const Akonadi::Item& item, const QSet<QByteArray>& parts)
{
    Q_UNUSED(parts)
    if (!checkItemAddedChanged<KAEvent>(item, CheckForChanged))
        return;
    QString errorMsg;
    if (mCompatibility != KACalendar::Current)
    {
        kWarning() << "Calendar not in current format";
        cancelTask(errorMessage(KAlarmResourceCommon::NotCurrentFormat));
        return;
    }
    KAEvent event = KAlarmResourceCommon::checkItemChanged(item, errorMsg);
    if (!event.isValid())
    {
        if (errorMsg.isEmpty())
            changeProcessed();
        else
            cancelTask(errorMsg);
        return;
    }

    KCalCore::Incidence::Ptr incidence = calendar()->incidence(item.remoteId());
    if (incidence)
    {
        if (incidence->isReadOnly())
        {
            kWarning() << "Event is read only:" << event.id();
            cancelTask(errorMessage(KAlarmResourceCommon::EventReadOnly, event.id()));
            return;
        }
        if (incidence->type() == KCalCore::Incidence::TypeEvent)
        {
            calendar()->deleteIncidence(incidence);   // it's not an Event
            incidence.clear();
        }
        else
        {
            KCalCore::Event::Ptr ev(incidence.staticCast<KCalCore::Event>());
            event.updateKCalEvent(ev, KAEvent::UID_SET);
            calendar()->setModified(true);
        }
    }
    if (!incidence)
    {
        // not in the calendar yet, should not happen -> add it
        KCalCore::Event::Ptr kcalEvent(new KCalCore::Event);
        event.updateKCalEvent(kcalEvent, KAEvent::UID_SET);
        calendar()->addIncidence(kcalEvent);
    }
    scheduleWrite();
    changeCommitted(item);
}

void KAlarmResource::collectionChanged(const Akonadi::Collection& collection)
{
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
void KAlarmResource::doRetrieveItems(const Akonadi::Collection& collection)
{
    kDebug();

    // Set the collection's compatibility status
    KAlarmResourceCommon::setCollectionCompatibility(collection, mCompatibility, mVersion);

    // Retrieve events from the calendar
    KCalCore::Event::List events = calendar()->events();
    Item::List items;
    foreach (const KCalCore::Event::Ptr& kcalEvent, events)
    {
        if (kcalEvent->alarms().isEmpty())
        {
            kWarning() << "KCalCore::Event has no alarms:" << kcalEvent->uid();
            continue;    // ignore events without alarms
        }

        KAEvent event(kcalEvent);
        QString mime = CalEvent::mimeType(event.category());
        if (mime.isEmpty())
        {
            kWarning() << "KAEvent has no alarms:" << event.id();
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
* Return the remote ID to use for the resource.
*/
QString KAlarmResource::remoteIdFromPath() const
{
    QString location(mSettings->path());
    KUrl url(location);
    return url.isLocalFile() ? url.toLocalFile() : location;
}

AKONADI_AGENT_FACTORY(KAlarmResource, akonadi_kalarm_resource)

#include "kalarmresource.moc"

// vim: et sw=4:
