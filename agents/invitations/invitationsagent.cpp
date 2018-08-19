/*
    Copyright 2009 Sebastian Sauer <sebsauer@kdab.net>

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

#include "invitationsagent.h"

#include "incidenceattribute.h"
#include "invitationagent_debug.h"

#include <AgentManager>
#include <ChangeRecorder>
#include <Collection>
#include <CollectionFetchJob>
#include <CollectionFetchScope>
#include <ItemFetchJob>
#include <ItemFetchScope>
#include <ItemModifyJob>
#include <Akonadi/KMime/MessageFlags>
#include <AkonadiCore/Session>
#include <resourcesynchronizationjob.h>
#include <specialcollections.h>
#include <specialcollectionsrequestjob.h>

#include <KCoreConfigSkeleton>
#include <KCalCore/Event>
#include <KCalCore/ICalFormat>
#include <KCalCore/Incidence>
#include <KCalCore/Journal>
#include <KCalCore/Todo>
#include <KConfig>
#include <KJob>
#include <KLocalizedString>
#include <KMime/Content>
#include <KMime/Message>

#include <QTimer>
#include <QDBusInterface>
#include <QDBusReply>
#include <QStandardPaths>
#include <QTimeZone>

using namespace Akonadi;

class InvitationsCollectionRequestJob : public SpecialCollectionsRequestJob
{
public:
    InvitationsCollectionRequestJob(SpecialCollections *collection, InvitationsAgent *agent)
        : SpecialCollectionsRequestJob(collection, agent)
    {
        setDefaultResourceType(QStringLiteral("akonadi_ical_resource"));

        QVariantMap options;
        options.insert(QStringLiteral("Path"), QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/akonadi_invitations")));
        options.insert(QStringLiteral("Name"), i18n("Invitations"));
        setDefaultResourceOptions(options);

        QMap<QByteArray, QString> displayNameMap;
        displayNameMap.insert("invitations", i18n("Invitations"));
        setTypes(displayNameMap.keys());
        setNameForTypeMap(displayNameMap);

        QMap<QByteArray, QString> iconNameMap;
        iconNameMap.insert("invitations", QStringLiteral("folder"));
        setIconForTypeMap(iconNameMap);
    }

    virtual ~InvitationsCollectionRequestJob()
    {
    }
};

class InvitationsCollection : public SpecialCollections
{
public:

    class Settings : public KCoreConfigSkeleton
    {
    public:
        Settings()
        {
            setCurrentGroup(QStringLiteral("Invitations"));
            addItemString(QStringLiteral("DefaultResourceId"), m_id, QString());
        }

        virtual ~Settings()
        {
        }

    private:
        QString m_id;
    };

    InvitationsCollection(InvitationsAgent *agent)
        : Akonadi::SpecialCollections(new Settings)
        , m_agent(agent)
        , sInvitationsType("invitations")
    {
    }

    virtual ~InvitationsCollection()
    {
    }

    void clear()
    {
        m_collection = Collection();
    }

    bool hasDefaultCollection() const
    {
        return SpecialCollections::hasDefaultCollection(sInvitationsType);
    }

    Collection defaultCollection() const
    {
        if (!m_collection.isValid()) {
            m_collection = SpecialCollections::defaultCollection(sInvitationsType);
        }

        return m_collection;
    }

    void registerDefaultCollection()
    {
        defaultCollection();
        registerCollection(sInvitationsType, m_collection);
    }

    SpecialCollectionsRequestJob *reguestJob() const
    {
        InvitationsCollectionRequestJob *job = new InvitationsCollectionRequestJob(const_cast<InvitationsCollection *>(this),
                                                                                   m_agent);
        job->requestDefaultCollection(sInvitationsType);

        return job;
    }

private:
    InvitationsAgent *m_agent = nullptr;
    const QByteArray sInvitationsType;
    mutable Collection m_collection;
};

InvitationsAgentItem::InvitationsAgentItem(InvitationsAgent *agent, const Item &originalItem)
    : QObject(agent)
    , m_agent(agent)
    , m_originalItem(originalItem)
{
}

InvitationsAgentItem::~InvitationsAgentItem()
{
}

void InvitationsAgentItem::add(const Item &item)
{
    const Collection collection = m_agent->collection();
    Q_ASSERT(collection.isValid());

    ItemCreateJob *job = new ItemCreateJob(item, collection, this);
    connect(job, &ItemCreateJob::result, this, &InvitationsAgentItem::createItemResult);

    m_jobs << job;

    job->start();
}

void InvitationsAgentItem::createItemResult(KJob *job)
{
    ItemCreateJob *createJob = qobject_cast<ItemCreateJob *>(job);
    m_jobs.removeAll(createJob);
    if (createJob->error()) {
        qCWarning(INVITATIONAGENT_LOG) << "Failed to create new Item in invitations collection." << createJob->errorText();
        return;
    }

    if (!m_jobs.isEmpty()) {
        return;
    }

    ItemFetchJob *fetchJob = new ItemFetchJob(m_originalItem, this);
    connect(fetchJob, &ItemFetchJob::result, this, &InvitationsAgentItem::fetchItemDone);
    fetchJob->start();
}

void InvitationsAgentItem::fetchItemDone(KJob *job)
{
    if (job->error()) {
        qCWarning(INVITATIONAGENT_LOG) << "Failed to fetch Item in invitations collection." << job->errorText();
        return;
    }

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob->items().count() == 1);

    Item modifiedItem = fetchJob->items().at(0);
    Q_ASSERT(modifiedItem.isValid());

    modifiedItem.setFlag(Akonadi::MessageFlags::HasInvitation);
    ItemModifyJob *modifyJob = new ItemModifyJob(modifiedItem, this);
    connect(modifyJob, &ItemModifyJob::result, this, &InvitationsAgentItem::modifyItemDone);
    modifyJob->start();
}

void InvitationsAgentItem::modifyItemDone(KJob *job)
{
    if (job->error()) {
        qCWarning(INVITATIONAGENT_LOG) << "Failed to modify Item in invitations collection." << job->errorText();
        return;
    }
}

InvitationsAgent::InvitationsAgent(const QString &id)
    : AgentBase(id)
    , AgentBase::ObserverV3()
    , m_invitationsCollection(new InvitationsCollection(this))
{
    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->itemFetchScope().fetchFullPayload();

    changeRecorder()->fetchCollection(true);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());
    changeRecorder()->setCollectionMonitored(Collection::root(), true);

    connect(this, &InvitationsAgent::reloadConfiguration, this, &InvitationsAgent::initStart);
    QTimer::singleShot(0, this, &InvitationsAgent::initStart);
}

InvitationsAgent::~InvitationsAgent()
{
    delete m_invitationsCollection;
}

void InvitationsAgent::initStart()
{
    m_invitationsCollection->clear();
    if (m_invitationsCollection->hasDefaultCollection()) {
        initDone();
    } else {
        SpecialCollectionsRequestJob *job = m_invitationsCollection->reguestJob();
        connect(job, &SpecialCollectionsRequestJob::result, this, &InvitationsAgent::initDone);
        job->start();
    }
}

void InvitationsAgent::initDone(KJob *job)
{
    if (job) {
        if (job->error()) {
            qCWarning(INVITATIONAGENT_LOG) << "Failed to request default collection:" << job->errorText();
            return;
        }

        m_invitationsCollection->registerDefaultCollection();
    }

    Q_ASSERT(m_invitationsCollection->defaultCollection().isValid());
    Q_EMIT status(AgentBase::Idle, i18n("Ready to dispatch invitations"));
}

Collection InvitationsAgent::collection() const
{
    return m_invitationsCollection->defaultCollection();
}

void InvitationsAgent::configure(WId windowId)
{
    qCDebug(INVITATIONAGENT_LOG) << windowId;
    initStart(); //reload
}

Item InvitationsAgent::handleContent(const QString &vcal, const KCalCore::MemoryCalendar::Ptr &calendar, const Item &item)
{
    KCalCore::ICalFormat format;
    KCalCore::ScheduleMessage::Ptr message = format.parseScheduleMessage(calendar, vcal);
    if (!message) {
        qCWarning(INVITATIONAGENT_LOG) << "Invalid invitation:" << vcal;
        return Item();
    }

    qCDebug(INVITATIONAGENT_LOG) << "id=" << item.id() << "remoteId=" << item.remoteId() << "vcal=" << vcal;

    KCalCore::Incidence::Ptr incidence = message->event().staticCast<KCalCore::Incidence>();
    Q_ASSERT(incidence);

    IncidenceAttribute *attr = new IncidenceAttribute;
    attr->setStatus(QStringLiteral("new"));   //TODO
    //attr->setFrom( message->from()->asUnicodeString() );
    attr->setReference(item.id());

    Item newItem;
    newItem.setMimeType(incidence->mimeType());
    newItem.addAttribute(attr);
    newItem.setPayload<KCalCore::Incidence::Ptr>(KCalCore::Incidence::Ptr(incidence->clone()));
    return newItem;
}

void InvitationsAgent::itemAdded(const Item &item, const Collection &collection)
{
    qCDebug(INVITATIONAGENT_LOG) << item.id() << collection;
    Q_UNUSED(collection);

    if (!m_invitationsCollection->defaultCollection().isValid()) {
        qCDebug(INVITATIONAGENT_LOG) << "No default collection found";
        return;
    }

    if (collection.isVirtual()) {
        return;
    }

    if (!item.hasPayload<KMime::Message::Ptr>()) {
        qCDebug(INVITATIONAGENT_LOG) << "Item has no payload";
        return;
    }

    KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();

    //TODO check if we are the sender and need to ignore the message...
    //const QString sender = message->sender()->asUnicodeString();
    //if ( identityManager()->thatIsMe( sender ) ) return;

    KCalCore::MemoryCalendar::Ptr calendar(new KCalCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    if (message->contentType()->isMultipart()) {
        qCDebug(INVITATIONAGENT_LOG) << "message is multipart:" << message->attachments().size();

        InvitationsAgentItem *it = nullptr;
        foreach (KMime::Content *content, message->contents()) {
            KMime::Headers::ContentType *ct = content->contentType();
            Q_ASSERT(ct);
            const QByteArray mimeType = ct->mimeType();
            qCDebug(INVITATIONAGENT_LOG) << "Mimetype of the body part is " << mimeType;
            if (mimeType != QByteArrayLiteral("text/calendar")) {
                continue;
            }

            Item newItem = handleContent(QLatin1String(content->body()), calendar, item);
            if (!newItem.hasPayload()) {
                qCDebug(INVITATIONAGENT_LOG) << "new item has no payload";
                continue;
            }

            if (!it) {
                it = new InvitationsAgentItem(this, item);
            }

            it->add(newItem);
        }
    } else {
        qCDebug(INVITATIONAGENT_LOG) << "message is not multipart";

        KMime::Headers::ContentType *ct = message->contentType();
        Q_ASSERT(ct);
        const QByteArray mimeType = ct->mimeType();
        qCDebug(INVITATIONAGENT_LOG) << "Mimetype of the body is " << mimeType;
        if (mimeType != QByteArrayLiteral("text/calendar")) {
            return;
        }

        qCDebug(INVITATIONAGENT_LOG) << "Message has an invitation in the body, processing";

        Item newItem = handleContent(QLatin1String(message->body()), calendar, item);
        if (!newItem.hasPayload()) {
            qCDebug(INVITATIONAGENT_LOG) << "new item has no payload";
            return;
        }

        InvitationsAgentItem *it = new InvitationsAgentItem(this, item);
        it->add(newItem);
    }
}

AKONADI_AGENT_MAIN(InvitationsAgent)
