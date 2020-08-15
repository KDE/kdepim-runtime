/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "resource.h"
#include "resource_debug.h"
#include "eventslistjob.h"
#include "birthdaylistjob.h"
#include "tokenjobs.h"

#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/CachePolicy>

#include <KLocalizedString>

#include <KCalendarCore/Event>

FacebookResource::FacebookResource(const QString &id)
    : Akonadi::ResourceBase(id)
{
    setNeedsNetwork(true);
    setName(i18n("Facebook"));
    connect(this, &FacebookResource::reloadConfiguration, this, &FacebookResource::synchronize);
}

FacebookResource::~FacebookResource()
{
}

void FacebookResource::abortActivity()
{
    if (mCurrentJob) {
        mCurrentJob->kill(KJob::EmitResult);
    }
}

void FacebookResource::cleanup()
{
    (new LogoutJob(identifier(), this))->exec();
    ResourceBase::cleanup();
}

Akonadi::Collection FacebookResource::makeCollection(Graph::RSVP rsvp, const QString &name, const Akonadi::Collection &parent)
{
    Akonadi::Collection col;
    col.setName(name);
    col.setRemoteId(Graph::rsvpToString(rsvp));
    col.setContentMimeTypes({ KCalendarCore::Event::eventMimeType() });
    col.setRights(Akonadi::Collection::ReadOnly);
    col.setParentCollection(parent);
    auto attr = col.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
    attr->setDisplayName(name);
    attr->setIconName(QStringLiteral("im-facebook"));

    return col;
}

void FacebookResource::retrieveCollections()
{
    QString userId;

    Akonadi::Collection root;
    root.setName(QStringLiteral("facebook_%1").arg(userId));
    root.setRemoteId(QStringLiteral("root"));
    root.setContentMimeTypes({ Akonadi::Collection::mimeType() });
    root.setRights(Akonadi::Collection::ReadOnly);
    root.setParentCollection(Akonadi::Collection::root());
    auto attr = root.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
    attr->setDisplayName(i18n("Facebook"));
    attr->setIconName(QStringLiteral("im-facebook"));

    collectionsRetrieved(
        { root,
          makeCollection(Graph::Attending, i18n("Events I'm Attending"), root),
          makeCollection(Graph::Declined, i18n("Events I'm not Attending"), root),
          makeCollection(Graph::MaybeAttending, i18n("Events I May Be Attending"), root),
          makeCollection(Graph::NotResponded, i18n("Events I have not Responded To"), root),
          makeCollection(Graph::Birthday, i18n("Friends' Birthdays"), root)});
}

void FacebookResource::retrieveItems(const Akonadi::Collection &collection)
{
    setItemStreamingEnabled(true);

    KJob *job = nullptr;
    if (Graph::rsvpFromString(collection.remoteId()) == Graph::Birthday) {
        job = new BirthdayListJob(identifier(), collection, this);
    } else {
        job = new EventsListJob(identifier(), collection, this);
        connect(static_cast<ListJob *>(job), &ListJob::itemsAvailable,
                this, [this](KJob *, const Akonadi::Item::List &items) {
            itemsRetrieved(items);
        });
    }
    connect(job, &KJob::result, this, &FacebookResource::onListJobDone);
    job->start();
    mCurrentJob = job;
}

bool FacebookResource::retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts)
{
    Q_UNUSED(items);
    Q_UNUSED(parts);

    // We always do full re-sync, and we always retrieve the entire payload, so
    // there's no need to implement this function
    return false;
}

void FacebookResource::onListJobDone(KJob *job)
{
    if (job->error()) {
        qCWarning(FBRESOURCE_LOG) << "Item sync error:" << job->errorString();
        cancelTask(job->errorString());
        return;
    }

    // Birthday job does not have item streaming
    if (auto bjob = qobject_cast<BirthdayListJob *>(job)) {
        itemsRetrieved(bjob->items());
    }

    itemsRetrievalDone();
}

int main(int argc, char **argv)
{
    // Enable to debug Facebook authentication
    //qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "8080");
    return Akonadi::ResourceBase::init<FacebookResource>(argc, argv);
}
