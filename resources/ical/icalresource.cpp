/*
    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2009 David Jarvie <djarvie@kde.org>
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

/*
    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2009 David Jarvie <djarvie@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "icalresource.h"
#include "icalsettingsadaptor.h"

#include <Akonadi/CollectionColorAttribute>
#include <Akonadi/ItemModifyJob>

#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/Incidence>

#include <KLocalizedString>

#include <QColor>
#include <QDBusConnection>
#include <QDebug>
#include <QTimeZone>

using namespace Akonadi;
using namespace KCalendarCore;
using namespace SETTINGS_NAMESPACE;

ICalResource::ICalResource(const QString &id)
    : SingleFileResource<Settings>(id)
{
    QStringList mimeTypes;
    mimeTypes << QStringLiteral("text/calendar") << KCalendarCore::Event::eventMimeType() << KCalendarCore::Todo::todoMimeType()
              << KCalendarCore::Journal::journalMimeType() << KCalendarCore::FreeBusy::freeBusyMimeType();
    setSupportedMimetypes(mimeTypes, QStringLiteral("office-calendar"));
    new ICalSettingsAdaptor(mSettings);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"), mSettings, QDBusConnection::ExportAdaptors);
}

ICalResource::~ICalResource() = default;

bool ICalResource::retrieveItems(const Akonadi::Item::List &items, [[maybe_unused]] const QSet<QByteArray> &parts)
{
    for (const Akonadi::Item &item : items) {
        qDebug() << "Item:" << item.url();
    }

    if (!mCalendar) {
        qCritical() << "akonadi_ical_resource: Calendar not loaded";
        Q_EMIT error(i18n("Calendar not loaded."));
        return false;
    }

    Akonadi::Item::List resultItems;
    resultItems.reserve(items.size());

    for (const Akonadi::Item &item : items) {
        const QString rid = item.remoteId();
        Incidence::Ptr incidence = calendar()->instance(rid);
        if (!incidence) {
            qCritical() << "akonadi_ical_resource: Can't find incidence with uid " << rid << "; item.id() = " << item.id();
            Q_EMIT error(i18n("Incidence with uid '%1' not found.", rid));
            return false;
        }

        Incidence::Ptr incidencePtr(incidence->clone());

        Item i = item;
        i.setMimeType(incidencePtr->mimeType());
        i.setPayload<Incidence::Ptr>(incidencePtr);
        resultItems.append(i);
    }

    itemsRetrieved(resultItems);
    return true;
}

bool ICalResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    return retrieveItems({item}, parts);
}

Collection ICalResource::rootCollection() const
{
    auto col = SingleFileResource::rootCollection();
    if (!calendar()) {
        return col;
    }
    if (mSettings->displayName().isEmpty() && !calendar()->name().isEmpty()) {
        col.attribute<EntityDisplayAttribute>(Collection::AddIfMissing)->setDisplayName(calendar()->name());
    }
#if KCALENDARCORE_VERSION >= QT_VERSION_CHECK(6, 26, 0)
    if (!calendar()->color().isEmpty()) {
        auto attr = col.attribute<CollectionColorAttribute>(Collection::AddIfMissing);
        attr->setColor(QColor::fromString(calendar()->color()));
    }
#endif
    if (const auto iconName = calendar()->nonKDECustomProperty("X-KDE-ICONNAME"); !iconName.isEmpty()) {
        auto attr = col.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setIconName(iconName);
    }
    return col;
}

void ICalResource::aboutToQuit()
{
    if (!mSettings->readOnly()) {
        writeFile();
    }
    mSettings->save();
}

bool ICalResource::readFromFile(const QString &fileName)
{
    mCalendar = KCalendarCore::MemoryCalendar::Ptr(new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    mFileStorage = KCalendarCore::FileStorage::Ptr(new KCalendarCore::FileStorage(mCalendar, fileName, new KCalendarCore::ICalFormat()));
    const bool result = mFileStorage->load();
    if (!result) {
        qCritical() << "akonadi_ical_resource: Error loading file " << fileName;
    }

    return result;
}

void ICalResource::itemRemoved(const Akonadi::Item &item)
{
    if (!mCalendar) {
        qCritical() << "akonadi_ical_resource: mCalendar is 0!";
        cancelTask(i18n("Calendar not loaded."));
        return;
    }

    const Incidence::Ptr i = mCalendar->instance(item.remoteId());
    if (i) {
        if (!mCalendar->deleteIncidence(i)) {
            qCritical() << "akonadi_ical_resource: Can't delete incidence with instance identifier " << item.remoteId() << "; item.id() = " << item.id();
            cancelTask();
            return;
        }
    } else {
        qCritical() << "akonadi_ical_resource: itemRemoved(): Can't find incidence with instance identifier " << item.remoteId()
                    << "; item.id() = " << item.id();
    }
    scheduleWrite();
    changeProcessed();
}

void ICalResource::retrieveItems(const Akonadi::Collection &col)
{
    reloadFile().then(this, [this, col](bool success) {
        qInfo() << "Reload file success:" << success;
        if (success) {
            if (mCalendar) {
                const Incidence::List incidences = calendar()->incidences();
                Item::List items;
                items.reserve(incidences.count());
                for (const Incidence::Ptr &incidence : incidences) {
                    Item item(incidence->mimeType());
                    item.setRemoteId(incidence->instanceIdentifier());
                    item.setPayload(Incidence::Ptr(incidence->clone()));
                    for (const auto &category : incidence->categories()) {
                        Tag tag(category);
                        tag.setRemoteId(category.toLatin1());
                        item.setTag(tag);
                    }
                    items << item;
                }
                itemsRetrieved(items);
            } else {
                qCritical() << "akonadi_ical_resource: retrieveItems(): mCalendar is 0!";
            }
        } else {
            qInfo() << "Reload file failed, not syncing items.";
            // Pretend nothing has changed
            itemsRetrievedIncremental({}, {});
            itemsRetrievalDone();
        }
    });
}

bool ICalResource::writeToFile(const QString &fileName)
{
    if (!mCalendar) {
        qCritical() << "akonadi_ical_resource: writeToFile() mCalendar is 0!";
        return false;
    }

    KCalendarCore::FileStorage *fileStorage = mFileStorage.data();
    if (fileName != mFileStorage->fileName()) {
        fileStorage = new KCalendarCore::FileStorage(mCalendar, fileName, new KCalendarCore::ICalFormat());
    }

    bool success = true;
    if (!fileStorage->save()) {
        qCritical() << QStringLiteral("akonadi_ical_resource: Failed to save calendar to file ") + fileName;
        Q_EMIT error(i18n("Failed to save calendar file to %1", fileName));
        success = false;
    }

    if (fileStorage != mFileStorage.data()) {
        delete fileStorage;
    }

    return success;
}

bool ICalResource::checkItemAddedChanged(const Akonadi::Item &item, CheckType type)
{
    if (!mCalendar) {
        cancelTask(i18n("Calendar not loaded."));
        return false;
    }
    if (!item.hasPayload<Incidence::Ptr>()) {
        const QString msg =
            (type == CheckForAdded) ? i18n("Unable to retrieve added item %1.", item.id()) : i18n("Unable to retrieve modified item %1.", item.id());
        cancelTask(msg);
        return false;
    }
    return true;
}

void ICalResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &)
{
    if (!checkItemAddedChanged(item, CheckForAdded)) {
        return;
    }

    auto i = item.payload<Incidence::Ptr>();
    if (!calendar()->addIncidence(Incidence::Ptr(i->clone()))) {
        // qCritical() << "akonadi_ical_resource: Error adding incidence with uid "
        //         << i->uid() << "; item.id() " << item.id() << i->recurrenceId();
        cancelTask();
        return;
    }

    Item it(item);
    it.setRemoteId(i->instanceIdentifier());
    scheduleWrite();
    changeCommitted(it);
}

void ICalResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    if (!checkItemAddedChanged(item, CheckForChanged)) {
        return;
    }

    auto payload = item.payload<Incidence::Ptr>();
    Incidence::Ptr incidence = calendar()->instance(item.remoteId());
    if (!incidence) {
        // not in the calendar yet, should not happen -> add it
        calendar()->addIncidence(Incidence::Ptr(payload->clone()));
    } else {
        // make sure any observer the resource might have installed gets properly notified
        incidence->startUpdates();

        if (incidence->type() == payload->type()) {
            // IncidenceBase::operator= calls virtual method assign, so it's safe.
            *incidence.staticCast<IncidenceBase>().data() = *payload.data();
            incidence->updated();
            incidence->endUpdates();
        } else {
            incidence->endUpdates();
            qWarning() << "akonadi_ical_resource: Item changed incidence type. Replacing it.";

            calendar()->deleteIncidence(incidence);
            calendar()->addIncidence(Incidence::Ptr(payload->clone()));
        }
    }
    scheduleWrite();
    changeCommitted(item);
}

namespace
{

void updateIncidenceCategories(Incidence::Ptr &incidence, const QSet<Akonadi::Tag> &tagsAdded, const QSet<Akonadi::Tag> &tagsRemoved)
{
    auto categories = incidence->categories();
    for (const auto &tag : tagsAdded) {
        categories.push_back(tag.name());
    }
    for (const auto &tag : tagsRemoved) {
        categories.removeAll(tag.name());
    }
    incidence->setCategories(categories);
}

} // namespace

void ICalResource::itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &tagsAdded, const QSet<Akonadi::Tag> &tagsRemoved)
{
    for (auto item : items) {
        if (!checkItemAddedChanged(item, CheckForChanged)) {
            return;
        }

        auto payload = item.payload<Incidence::Ptr>();
        Incidence::Ptr incidence = calendar()->instance(item.remoteId());
        if (!incidence) {
            // not in calendar yet, should not happen -> add it
            auto newPayload = Incidence::Ptr(payload->clone());
            updateIncidenceCategories(newPayload, tagsAdded, tagsRemoved);
            calendar()->addIncidence(newPayload);
            item.setPayload(newPayload);
        } else {
            incidence->startUpdates();
            updateIncidenceCategories(incidence, tagsAdded, tagsRemoved);
            incidence->endUpdates();
            item.setPayload(incidence);
        }

        // We have changed the payload, so we need to upload the change back to Akonadi
        auto update = new Akonadi::ItemModifyJob(item, this);
        connect(update, &KJob::result, this, [](KJob *job) {
            if (job->error()) {
                qWarning() << "Failed to update item" << job->errorString();
            }
        });
    }

    scheduleWrite();
    changesCommitted(items);
}

void ICalResource::collectionChanged(const Akonadi::Collection &col)
{
    if (!readOnly() && calendar()) {
        if (calendar()->name() != col.displayName()) {
            calendar()->setName(col.displayName());
            scheduleWrite();
        }
#if KCALENDARCORE_VERSION >= QT_VERSION_CHECK(6, 26, 0)
        if (const auto attr = col.attribute<CollectionColorAttribute>(); attr && attr->color().isValid()) {
            calendar()->setColor(attr->color().name());
            scheduleWrite();
        }
#endif
        if (const auto attr = col.attribute<EntityDisplayAttribute>(); attr && !attr->iconName().isEmpty()) {
            calendar()->setNonKDECustomProperty("X-KDE-ICONNAME", attr->iconName());
            scheduleWrite();
        }
    }
    SingleFileResource::collectionChanged(col);
}

KCalendarCore::MemoryCalendar::Ptr ICalResource::calendar() const
{
    return mCalendar;
}

AKONADI_RESOURCE_CORE_MAIN(ICalResource)

#include "moc_icalresource.cpp"
