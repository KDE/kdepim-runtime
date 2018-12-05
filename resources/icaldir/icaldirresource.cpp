/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Bertjan Broeksema <broeksema@kde.org>
    Copyright (c) 2012 Sérgio Martins <iamsergio@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "icaldirresource.h"

#include "settingsadaptor.h"

#include <changerecorder.h>
#include <entitydisplayattribute.h>
#include <itemfetchscope.h>

#include <KCalCore/MemoryCalendar>
#include <KCalCore/FileStorage>
#include <KCalCore/ICalFormat>
#include <QIcon>
#include <KLocalizedString>
#include <QDebug>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTimeZone>

using namespace Akonadi;
using namespace KCalCore;

static Incidence::Ptr readFromFile(const QString &fileName, const QString &expectedIdentifier)
{
    MemoryCalendar::Ptr calendar = MemoryCalendar::Ptr(new MemoryCalendar(QTimeZone::utc()));
    FileStorage::Ptr fileStorage = FileStorage::Ptr(new FileStorage(calendar, fileName, new ICalFormat()));

    Incidence::Ptr incidence;
    if (fileStorage->load()) {
        Incidence::List incidences = calendar->incidences();
        if (incidences.count() == 1 && incidences.first()->instanceIdentifier() == expectedIdentifier) {
            incidence = incidences.first();
        }
    } else {
        qCritical() << "Error loading file " << fileName;
    }

    return incidence;
}

static bool writeToFile(const QString &fileName, Incidence::Ptr &incidence)
{
    if (!incidence) {
        qCritical() << "incidence is 0!";
        return false;
    }

    MemoryCalendar::Ptr calendar = MemoryCalendar::Ptr(new MemoryCalendar(QTimeZone::utc()));
    FileStorage::Ptr fileStorage = FileStorage::Ptr(new FileStorage(calendar, fileName, new ICalFormat()));
    calendar->addIncidence(incidence);
    Q_ASSERT(calendar->incidences().count() == 1);

    const bool success = fileStorage->save();
    if (!success) {
        qCritical() << QStringLiteral("Failed to save calendar to file ") + fileName;
    }

    return success;
}

ICalDirResource::ICalDirResource(const QString &id)
    : ResourceBase(id)
{
    IcalDirResourceSettings::instance(KSharedConfig::openConfig());
    // setup the resource
    new SettingsAdaptor(IcalDirResourceSettings::self());
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 IcalDirResourceSettings::self(), QDBusConnection::ExportAdaptors);

    changeRecorder()->itemFetchScope().fetchFullPayload();
    connect(this, &ICalDirResource::reloadConfiguration, this, &ICalDirResource::slotReloadConfig);
}

ICalDirResource::~ICalDirResource()
{
}

void ICalDirResource::slotReloadConfig()
{
    initializeICalDirectory();
    loadIncidences();

    synchronize();
}

void ICalDirResource::aboutToQuit()
{
    IcalDirResourceSettings::self()->save();
}

bool ICalDirResource::loadIncidences()
{
    mIncidences.clear();

    QDirIterator it(iCalDirectoryName());
    while (it.hasNext()) {
        it.next();
        if (it.fileName() != QLatin1String(".") && it.fileName() != QLatin1String("..") && it.fileName() != QLatin1String("WARNING_README.txt")) {
            const KCalCore::Incidence::Ptr incidence = readFromFile(it.filePath(), it.fileName());
            if (incidence) {
                mIncidences.insert(incidence->instanceIdentifier(), incidence);
            }
        }
    }

    Q_EMIT status(Idle);
    return true;
}

bool ICalDirResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    const QString remoteId = item.remoteId();
    if (!mIncidences.contains(remoteId)) {
        Q_EMIT error(i18n("Incidence with uid '%1' not found.", remoteId));
        return false;
    }

    Item newItem(item);
    newItem.setPayload<KCalCore::Incidence::Ptr>(mIncidences.value(remoteId));
    itemRetrieved(newItem);

    return true;
}

void ICalDirResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &)
{
    if (IcalDirResourceSettings::self()->readOnly()) {
        Q_EMIT error(i18n("Trying to write to a read-only directory: '%1'", iCalDirectoryName()));
        cancelTask();
        return;
    }

    KCalCore::Incidence::Ptr incidence;
    if (item.hasPayload<KCalCore::Incidence::Ptr>()) {
        incidence = item.payload<KCalCore::Incidence::Ptr>();
    }

    if (incidence) {
        // add it to the cache...
        mIncidences.insert(incidence->instanceIdentifier(), incidence);

        // ... and write it through to the file system
        const bool success = writeToFile(iCalDirectoryFileName(incidence->instanceIdentifier()), incidence);

        if (success) {
            // report everything ok
            Item newItem(item);
            newItem.setRemoteId(incidence->instanceIdentifier());
            changeCommitted(newItem);
        } else {
            cancelTask();
        }
    } else {
        changeProcessed();
    }
}

void ICalDirResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    if (IcalDirResourceSettings::self()->readOnly()) {
        Q_EMIT error(i18n("Trying to write to a read-only directory: '%1'", iCalDirectoryName()));
        cancelTask();
        return;
    }

    KCalCore::Incidence::Ptr incidence;
    if (item.hasPayload<KCalCore::Incidence::Ptr>()) {
        incidence = item.payload<KCalCore::Incidence::Ptr>();
    }

    if (incidence) {
        // change it in the cache...
        mIncidences.insert(incidence->instanceIdentifier(), incidence);

        // ... and write it through to the file system
        const bool success = writeToFile(iCalDirectoryFileName(incidence->instanceIdentifier()), incidence);

        if (success) {
            Item newItem(item);
            newItem.setRemoteId(incidence->instanceIdentifier());
            changeCommitted(newItem);
        } else {
            cancelTask();
        }
    } else {
        changeProcessed();
    }
}

void ICalDirResource::itemRemoved(const Akonadi::Item &item)
{
    if (IcalDirResourceSettings::self()->readOnly()) {
        Q_EMIT error(i18n("Trying to write to a read-only directory: '%1'", iCalDirectoryName()));
        cancelTask();
        return;
    }

    // remove it from the cache...
    mIncidences.remove(item.remoteId());

    // ... and remove it from the file system
    QFile::remove(iCalDirectoryFileName(item.remoteId()));

    changeProcessed();
}

void ICalDirResource::collectionChanged(const Collection &collection)
{
    if (collection.hasAttribute<EntityDisplayAttribute>()) {
        auto attr = collection.attribute<EntityDisplayAttribute>();
        if (attr->displayName() != name()) {
            setName(attr->displayName());
        }
    }

    changeProcessed();
}

void ICalDirResource::retrieveCollections()
{
    Collection c;
    c.setParentCollection(Collection::root());
    c.setRemoteId(iCalDirectoryName());
    c.setName(name());

    QStringList mimetypes;
    mimetypes << KCalCore::Event::eventMimeType() << KCalCore::Todo::todoMimeType() << KCalCore::Journal::journalMimeType() << QStringLiteral("text/calendar");
    c.setContentMimeTypes(mimetypes);

    if (IcalDirResourceSettings::self()->readOnly()) {
        c.setRights(Collection::CanChangeCollection);
    } else {
        Collection::Rights rights = Collection::ReadOnly;
        rights |= Collection::CanChangeItem;
        rights |= Collection::CanCreateItem;
        rights |= Collection::CanDeleteItem;
        rights |= Collection::CanChangeCollection;
        c.setRights(rights);
    }

    EntityDisplayAttribute *attr = c.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(name() == identifier() ? i18n("Calendar Folder") : name());
    attr->setIconName(QStringLiteral("office-calendar"));

    Collection::List list;
    list << c;
    collectionsRetrieved(list);
}

void ICalDirResource::retrieveItems(const Akonadi::Collection &)
{
    loadIncidences();
    Item::List items;
    items.reserve(mIncidences.count());

    for (const KCalCore::Incidence::Ptr &incidence : qAsConst(mIncidences)) {
        Item item;
        item.setRemoteId(incidence->instanceIdentifier());
        item.setMimeType(incidence->mimeType());
        items.append(item);
    }

    itemsRetrieved(items);
}

QString ICalDirResource::iCalDirectoryName() const
{
    return IcalDirResourceSettings::self()->path();
}

QString ICalDirResource::iCalDirectoryFileName(const QString &file) const
{
    return IcalDirResourceSettings::self()->path() + QDir::separator() + file;
}

void ICalDirResource::initializeICalDirectory() const
{
    QDir dir(iCalDirectoryName());

    // if folder does not exists, create it
    if (!dir.exists()) {
        QDir::root().mkpath(dir.absolutePath());
    }

    // check whether warning file is in place...
    QFile file(dir.absolutePath() + QDir::separator() + QStringLiteral("WARNING_README.txt"));
    if (!file.exists()) {
        // ... if not, create it
        file.open(QIODevice::WriteOnly);
        file.write("Important Warning!!!\n\n"
                   "Don't create or copy files inside this folder manually, they are managed by the Akonadi framework!\n");
        file.close();
    }
}

AKONADI_RESOURCE_MAIN(ICalDirResource)
