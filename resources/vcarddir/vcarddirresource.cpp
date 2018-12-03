/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Bertjan Broeksema <broeksema@kde.org>

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

#include "vcarddirresource.h"

#include "settingsadaptor.h"
#include "vcarddirresource_debug.h"
#include "dirsettingsdialog.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>

#include <changerecorder.h>
#include <entitydisplayattribute.h>
#include <itemfetchscope.h>

#include <QIcon>
#include <KLocalizedString>
#include <QDebug>

using namespace Akonadi;

VCardDirResource::VCardDirResource(const QString &id)
    : ResourceBase(id)
{
    // setup the resource
    new SettingsAdaptor(Settings::self());
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 Settings::self(), QDBusConnection::ExportAdaptors);

    changeRecorder()->itemFetchScope().fetchFullPayload();
}

VCardDirResource::~VCardDirResource()
{
    // clear cache
    mAddressees.clear();
}

void VCardDirResource::aboutToQuit()
{
    Settings::self()->save();
}

void VCardDirResource::configure(WId windowId)
{
    SettingsDialog dlg(windowId);
    dlg.setWindowIcon(QIcon::fromTheme(QStringLiteral("text-directory")));
    if (dlg.exec()) {
        initializeVCardDirectory();
        loadAddressees();

        synchronize();

        Q_EMIT configurationDialogAccepted();
    } else {
        Q_EMIT configurationDialogRejected();
    }
}

bool VCardDirResource::loadAddressees()
{
    mAddressees.clear();

    QDirIterator it(vCardDirectoryName());
    while (it.hasNext()) {
        it.next();
        if (it.fileName() != QLatin1String(".") && it.fileName() != QLatin1String("..") && it.fileName() != QLatin1String("WARNING_README.txt")) {
            QFile file(it.filePath());
            if (file.open(QIODevice::ReadOnly)) {
                const QByteArray data = file.readAll();
                file.close();

                const KContacts::Addressee addr = mConverter.parseVCard(data);
                if (!addr.isEmpty()) {
                    mAddressees.insert(addr.uid(), addr);
                }
            } else {
                qCCritical(VCARDDIRRESOURCE_LOG) << " file can't be load " << it.filePath();
            }
        }
    }

    Q_EMIT status(Idle);

    return true;
}

bool VCardDirResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    const QString remoteId = item.remoteId();
    if (!mAddressees.contains(remoteId)) {
        Q_EMIT error(i18n("Contact with uid '%1' not found.", remoteId));
        return false;
    }

    Item newItem(item);
    newItem.setPayload<KContacts::Addressee>(mAddressees.value(remoteId));
    itemRetrieved(newItem);

    return true;
}

void VCardDirResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &)
{
    if (Settings::self()->readOnly()) {
        Q_EMIT error(i18n("Trying to write to a read-only directory: '%1'", vCardDirectoryName()));
        cancelTask();
        return;
    }

    KContacts::Addressee addressee;
    if (item.hasPayload<KContacts::Addressee>()) {
        addressee = item.payload<KContacts::Addressee>();
    }

    if (!addressee.isEmpty()) {
        // add it to the cache...
        mAddressees.insert(addressee.uid(), addressee);

        // ... and write it through to the file system
        const QByteArray data = mConverter.createVCard(addressee);

        QFile file(vCardDirectoryFileName(addressee.uid()));
        file.open(QIODevice::WriteOnly);
        file.write(data);
        file.close();

        // report everything ok
        Item newItem(item);
        newItem.setRemoteId(addressee.uid());
        changeCommitted(newItem);
    } else {
        changeProcessed();
    }
}

void VCardDirResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    if (Settings::self()->readOnly()) {
        Q_EMIT error(i18n("Trying to write to a read-only directory: '%1'", vCardDirectoryName()));
        cancelTask();
        return;
    }

    KContacts::Addressee addressee;
    if (item.hasPayload<KContacts::Addressee>()) {
        addressee = item.payload<KContacts::Addressee>();
    }

    if (!addressee.isEmpty()) {
        // change it in the cache...
        mAddressees.insert(addressee.uid(), addressee);

        // ... and write it through to the file system
        const QByteArray data = mConverter.createVCard(addressee);

        QFile file(vCardDirectoryFileName(addressee.uid()));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();

            Item newItem(item);
            newItem.setRemoteId(addressee.uid());
            changeCommitted(newItem);
        } else {
            qCritical() << " We can't write in file " << file.fileName();
        }
    } else {
        changeProcessed();
    }
}

void VCardDirResource::itemRemoved(const Akonadi::Item &item)
{
    if (Settings::self()->readOnly()) {
        Q_EMIT error(i18n("Trying to write to a read-only directory: '%1'", vCardDirectoryName()));
        cancelTask();
        return;
    }

    // remove it from the cache...
    mAddressees.remove(item.remoteId());

    // ... and remove it from the file system
    QFile::remove(vCardDirectoryFileName(item.remoteId()));

    changeProcessed();
}

void VCardDirResource::retrieveCollections()
{
    Collection c;
    c.setParentCollection(Collection::root());
    c.setRemoteId(vCardDirectoryName());
    c.setName(name());
    QStringList mimeTypes;
    mimeTypes << KContacts::Addressee::mimeType();
    c.setContentMimeTypes(mimeTypes);
    if (Settings::self()->readOnly()) {
        c.setRights(Collection::ReadOnly);
    } else {
        Collection::Rights rights;
        rights |= Collection::CanChangeItem;
        rights |= Collection::CanCreateItem;
        rights |= Collection::CanDeleteItem;
        rights |= Collection::CanChangeCollection;
        c.setRights(rights);
    }

    EntityDisplayAttribute *attr = c.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(i18n("Contacts Folder"));
    attr->setIconName(QStringLiteral("x-office-address-book"));

    Collection::List list;
    list << c;
    collectionsRetrieved(list);
}

void VCardDirResource::retrieveItems(const Akonadi::Collection &)
{
    Item::List items;
    items.reserve(mAddressees.count());

    for (const KContacts::Addressee &addressee : qAsConst(mAddressees)) {
        Item item;
        item.setRemoteId(addressee.uid());
        item.setMimeType(KContacts::Addressee::mimeType());
        items.append(item);
    }

    itemsRetrieved(items);
}

QString VCardDirResource::vCardDirectoryName() const
{
    return Settings::self()->path();
}

QString VCardDirResource::vCardDirectoryFileName(const QString &file) const
{
    return Settings::self()->path() + QDir::separator() + file;
}

void VCardDirResource::initializeVCardDirectory() const
{
    QDir dir(vCardDirectoryName());

    // if folder does not exists, create it
    if (!dir.exists()) {
        QDir::root().mkpath(dir.absolutePath());
    }

    // check whether warning file is in place...
    QFile file(dir.absolutePath() + QDir::separator() + QLatin1String("WARNING_README.txt"));
    if (!file.exists()) {
        // ... if not, create it
        file.open(QIODevice::WriteOnly);
        file.write("Important Warning!!!\n\n"
                   "Don't create or copy vCards inside this folder manually, they are managed by the Akonadi framework!\n");
        file.close();
    }
}

AKONADI_RESOURCE_MAIN(VCardDirResource)
