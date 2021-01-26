/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2008 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "vcardresource.h"
#include "vcardsettingsadaptor.h"

#include <KLocalizedString>

#include <QDBusConnection>

using namespace Akonadi;
using namespace Akonadi_VCard_Resource;

VCardResource::VCardResource(const QString &id)
    : SingleFileResource<Settings>(id)
{
    setSupportedMimetypes(QStringList() << KContacts::Addressee::mimeType(), QStringLiteral("office-address-book"));

    new VCardSettingsAdaptor(mSettings);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"), mSettings, QDBusConnection::ExportAdaptors);
}

VCardResource::~VCardResource()
{
    mAddressees.clear();
}

bool VCardResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)
    const QString rid = item.remoteId();
    if (!mAddressees.contains(rid)) {
        Q_EMIT error(i18n("Contact with uid '%1' not found.", rid));
        return false;
    }
    Item i(item);
    i.setPayload<KContacts::Addressee>(mAddressees.value(rid));
    itemRetrieved(i);
    return true;
}

void VCardResource::aboutToQuit()
{
    if (!mSettings->readOnly()) {
        writeFile();
    }
    mSettings->save();
}

void VCardResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &)
{
    KContacts::Addressee addressee;
    if (item.hasPayload<KContacts::Addressee>()) {
        addressee = item.payload<KContacts::Addressee>();
    }

    if (!addressee.isEmpty()) {
        mAddressees.insert(addressee.uid(), addressee);

        Item i(item);
        i.setRemoteId(addressee.uid());
        changeCommitted(i);

        scheduleWrite();
    } else {
        changeProcessed();
    }
}

void VCardResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    KContacts::Addressee addressee;
    if (item.hasPayload<KContacts::Addressee>()) {
        addressee = item.payload<KContacts::Addressee>();
    }

    if (!addressee.isEmpty()) {
        mAddressees.insert(addressee.uid(), addressee);

        Item i(item);
        i.setRemoteId(addressee.uid());
        changeCommitted(i);

        scheduleWrite();
    } else {
        changeProcessed();
    }
}

void VCardResource::itemRemoved(const Akonadi::Item &item)
{
    mAddressees.remove(item.remoteId());

    scheduleWrite();

    changeProcessed();
}

void VCardResource::retrieveItems(const Akonadi::Collection &col)
{
    // vCard does not support folders so we can safely ignore the collection
    Q_UNUSED(col)

    Item::List items;
    items.reserve(mAddressees.count());

    // FIXME: Check if the KIO::Job is done and was successful, if so send the
    // items, otherwise set a bool and in the result slot of the job send the
    // items if the bool is set.

    for (const KContacts::Addressee &addressee : qAsConst(mAddressees)) {
        Item item;
        item.setRemoteId(addressee.uid());
        item.setMimeType(KContacts::Addressee::mimeType());
        item.setPayload(addressee);
        items.append(item);
    }

    itemsRetrieved(items);
}

bool VCardResource::readFromFile(const QString &fileName)
{
    mAddressees.clear();

    QFile file(QUrl::fromLocalFile(fileName).toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        Q_EMIT status(Broken, i18n("Unable to open vCard file '%1'.", fileName));
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    const KContacts::Addressee::List list = mConverter.parseVCards(data);
    const int numberOfElementInList = list.count();
    for (int i = 0; i < numberOfElementInList; ++i) {
        mAddressees.insert(list[i].uid(), list[i]);
    }

    return true;
}

bool VCardResource::writeToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        Q_EMIT status(Broken, i18n("Unable to open vCard file '%1'.", fileName));
        return false;
    }

    QVector<KContacts::Addressee> v;
    v.reserve(mAddressees.size());
    for (const KContacts::Addressee &addressee : qAsConst(mAddressees)) {
        v.push_back(addressee);
    }

    const QByteArray data = mConverter.createVCards(v);

    file.write(data);
    file.close();

    return true;
}

AKONADI_RESOURCE_MAIN(VCardResource)
