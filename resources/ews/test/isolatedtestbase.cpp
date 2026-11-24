/*
    SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "isolatedtestbase.h"

#include <QDomDocument>
#include <QDomElement>
#include <QHash>
#include <QTest>

#include <algorithm>

#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentManager>
#include <Akonadi/Control>

#include "ewsresourceinterface.h"
#include "ewssettingsinterface.h"
#include "ewswalletinterface.h"
#include "fakeewsserverthread.h"

using namespace Akonadi;
using namespace Qt::Literals::StringLiterals;

constexpr int OnlineStateChangeTimeoutMs = 20000;

namespace
{
// The local name of an element, whether or not the document uses a namespace prefix.
QString localName(const QDomElement &element)
{
    const QString tagName = element.tagName();
    const qsizetype colon = tagName.indexOf(u':');
    return colon < 0 ? tagName : tagName.mid(colon + 1);
}

bool isNamed(const QDomElement &element, const QString &name)
{
    return !element.isNull() && localName(element) == name;
}

// The first element below root that carries the given local name, or a null
// element when there is none.
QDomElement firstDescendant(const QDomElement &root, const QString &name)
{
    for (QDomElement element = root.firstChildElement(); !element.isNull(); element = element.nextSiblingElement()) {
        if (isNamed(element, name)) {
            return element;
        }
        if (const QDomElement found = firstDescendant(element, name); !found.isNull()) {
            return found;
        }
    }
    return {};
}

// The child elements of parent with the given local name, in document order.
QList<QDomElement> childElements(const QDomElement &parent, const QString &name)
{
    QList<QDomElement> elements;
    for (QDomElement element = parent.firstChildElement(); !element.isNull(); element = element.nextSiblingElement()) {
        if (isNamed(element, name)) {
            elements.append(element);
        }
    }
    return elements;
}

QDomElement firstChild(const QDomElement &parent, const QString &name)
{
    return childElements(parent, name).value(0);
}

QString elementText(const QDomElement &element)
{
    return element.isNull() ? QString() : element.text();
}

bool hasAttribute(const QDomElement &element, const QString &name, const QString &value)
{
    return !element.isNull() && element.attribute(name) == value;
}

// The <soap:Body> of an EWS request, or a null element for anything else.
QDomElement requestBody(const QDomElement &request)
{
    const QDomElement envelope = isNamed(request, u"Envelope"_s) ? request : firstDescendant(request, u"Envelope"_s);
    return firstChild(envelope, u"Body"_s);
}

/* The GetFolder request in the given request, when it asks for the given base shape, and a
 * null element for any other request. */
QDomElement getFolderRequest(const QDomElement &request, const QString &baseShape)
{
    const QDomElement getFolder = firstChild(requestBody(request), u"GetFolder"_s);
    if (elementText(firstChild(firstChild(getFolder, u"FolderShape"_s), u"BaseShape"_s)) != baseShape) {
        return {};
    }
    return getFolder;
}

/* True when the GetFolder request asks for exactly the given distinguished folder ids in
 * this order and for no other folder ids. */
bool asksForDistinguishedFolders(const QDomElement &getFolder, const QStringList &ids)
{
    const QDomElement folderIds = firstChild(getFolder, u"FolderIds"_s);
    const QList<QDomElement> distinguished = childElements(folderIds, u"DistinguishedFolderId"_s);
    if (distinguished.size() != ids.size() || !childElements(folderIds, u"FolderId"_s).isEmpty()) {
        return false;
    }
    for (qsizetype i = 0; i < ids.size(); ++i) {
        if (distinguished.at(i).attribute(u"Id"_s) != ids.at(i)) {
            return false;
        }
    }
    return true;
}

// The additional properties a request asks for, or a null element when it asks for none.
QDomElement additionalProperties(const QDomElement &request)
{
    return firstDescendant(request, u"AdditionalProperties"_s);
}

bool asksForTagProperties(const QDomElement &request)
{
    const QList<QDomElement> extendedUris = childElements(additionalProperties(request), u"ExtendedFieldURI"_s);
    if (extendedUris.size() != 2) {
        return false;
    }
    const auto isTagProperty = [](const QDomElement &property, const QString &name, const QString &type) {
        return hasAttribute(property, u"PropertySetId"_s, u"9bf757ae-69b5-4d8a-bf1d-2dd0c0871a28"_s) && hasAttribute(property, u"PropertyName"_s, name)
            && hasAttribute(property, u"PropertyType"_s, type);
    };
    return isTagProperty(extendedUris.at(0), u"GlobalTags"_s, u"StringArray"_s) && isTagProperty(extendedUris.at(1), u"GlobalTagsVersion"_s, u"Integer"_s);
}

// The response envelope every dialog entry below answers with.
const QString responseEnvelopeStart = QStringLiteral(
    "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
    "xmlns:m=\"http://schemas.microsoft.com/exchange/services/2006/messages\" "
    "xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
    "<soap:Header>"
    "<t:ServerVersionInfo MajorVersion=\"15\" MinorVersion=\"01\" MajorBuildNumber=\"225\" MinorBuildNumber=\"042\" />"
    "</soap:Header><soap:Body>");
const QString responseEnvelopeEnd = QStringLiteral("</soap:Body></soap:Envelope>");

QString getFolderResponseMessage(const QString &folderId)
{
    return QStringLiteral(
               "<m:GetFolderResponseMessage ResponseClass=\"Success\">"
               "<m:ResponseCode>NoError</m:ResponseCode>"
               "<m:Folders><t:Folder>"
               "<t:FolderId Id=\"%1\" ChangeKey=\"MDAx\" />"
               "</t:Folder></m:Folders>"
               "</m:GetFolderResponseMessage>")
        .arg(folderId);
}
} // namespace

IsolatedTestBase::IsolatedTestBase(QObject *parent)
    : QObject(parent)
    , mFakeServerThread(new FakeEwsServerThread(this))
{
}

IsolatedTestBase::~IsolatedTestBase() = default;

void IsolatedTestBase::init()
{
    QVERIFY(Control::start());

    /* Switch all resources offline to reduce interference from them */
    for (AgentInstance agent : AgentManager::self()->instances()) {
        agent.setIsOnline(false);
    }

    connect(AgentManager::self(), &AgentManager::instanceAdded, this, [](const Akonadi::AgentInstance &instance) {
        qDebug() << "AgentInstanceAdded" << instance.identifier();
    });

    mFakeServerThread->start();
    QVERIFY(mFakeServerThread->waitServerStarted());
}

void IsolatedTestBase::cleanup()
{
    mFakeServerThread->exit();
    mFakeServerThread->wait();
}

QString IsolatedTestBase::loadResourceAsString(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(f.readAll());
    }
    qWarning() << "Resource" << path << "not found";
    return {};
}

template<typename T>
QDBusReply<T> dBusSetAndWaitReply(std::function<QDBusReply<T>()> setFunc, std::function<QDBusReply<T>()> getFunc, const QString &name)
{
    QDBusReply<T> reply;
    int retryCnt = 4;
    do {
        setFunc();
        reply = getFunc();
        if (!reply.isValid()) {
            qDebug() << "Failed to set DBus config option" << name << reply.error().message();
            QThread::msleep(250);
        }
    } while (!reply.isValid() && retryCnt-- > 0);

    return reply;
}

TestAgentInstance::TestAgentInstance(const QString &url)
{
    AgentType ewsType = AgentManager::self()->type(QStringLiteral("akonadi_ews_resource"));
    auto agentCreateJob = new AgentInstanceCreateJob(ewsType);
    QVERIFY(agentCreateJob->exec());

    mEwsInstance.reset(new AgentInstance(agentCreateJob->instance()));
    mIdentifier = mEwsInstance->identifier();
    const QString akonadiInstanceIdentifier = QProcessEnvironment::systemEnvironment().value(QStringLiteral("AKONADI_INSTANCE"));

    mEwsSettingsInterface.reset(
        new OrgKdeAkonadiEwsSettingsInterface(QStringLiteral("org.freedesktop.Akonadi.Resource.") + mIdentifier + u'.' + akonadiInstanceIdentifier,
                                              QStringLiteral("/Settings"),
                                              QDBusConnection::sessionBus(),
                                              this));
    QVERIFY(mEwsSettingsInterface->isValid());

    mEwsWalletInterface.reset(
        new OrgKdeAkonadiEwsWalletInterface(QStringLiteral("org.freedesktop.Akonadi.Resource.") + mIdentifier + u'.' + akonadiInstanceIdentifier,
                                            QStringLiteral("/Settings"),
                                            QDBusConnection::sessionBus(),
                                            this));
    QVERIFY(mEwsWalletInterface->isValid());

    mEwsResourceInterface.reset(
        new OrgKdeAkonadiEwsResourceInterface(QStringLiteral("org.freedesktop.Akonadi.Resource.") + mIdentifier + u'.' + akonadiInstanceIdentifier,
                                              QStringLiteral("/"),
                                              QDBusConnection::sessionBus(),
                                              this));
    QVERIFY(mEwsResourceInterface->isValid());

    /* The EWS resource initializes its DBus adapters asynchronously. Therefore it can happen that
     * due to a race access is attempted prior to their initialization. To fix this retry the DBus
     * communication a few times before declaring failure. */
    const auto baseUrlReply = dBusSetAndWaitReply<QString>(std::bind(&OrgKdeAkonadiEwsSettingsInterface::setBaseUrl, mEwsSettingsInterface.data(), url),
                                                           std::bind(&OrgKdeAkonadiEwsSettingsInterface::baseUrl, mEwsSettingsInterface.data()),
                                                           QStringLiteral("Base URL"));
    QVERIFY(baseUrlReply.isValid());
    QCOMPARE(baseUrlReply.value(), url);

    const auto hasDomainReply = dBusSetAndWaitReply<bool>(std::bind(&OrgKdeAkonadiEwsSettingsInterface::setHasDomain, mEwsSettingsInterface.data(), false),
                                                          std::bind(&OrgKdeAkonadiEwsSettingsInterface::hasDomain, mEwsSettingsInterface.data()),
                                                          QStringLiteral("has domain"));
    QVERIFY(hasDomainReply.isValid());
    QCOMPARE(hasDomainReply.value(), false);

    const auto username = QStringLiteral("test");
    const auto usernameReply = dBusSetAndWaitReply<QString>(std::bind(&OrgKdeAkonadiEwsSettingsInterface::setUsername, mEwsSettingsInterface.data(), username),
                                                            std::bind(&OrgKdeAkonadiEwsSettingsInterface::username, mEwsSettingsInterface.data()),
                                                            QStringLiteral("Username"));
    QVERIFY(usernameReply.isValid());
    QCOMPARE(usernameReply.value(), username);

    mEwsWalletInterface->setTestPassword(QStringLiteral("test"));
    AgentManager::self()->instance(mIdentifier).reconfigure();
}

TestAgentInstance::~TestAgentInstance()
{
    if (mEwsInstance) {
        AgentManager::self()->removeInstance(*mEwsInstance);
    }
}

bool TestAgentInstance::isValid() const
{
    return mEwsInstance && mEwsSettingsInterface && mEwsWalletInterface && !mIdentifier.isEmpty();
}

const QString &TestAgentInstance::identifier() const
{
    return mIdentifier;
}

bool TestAgentInstance::setOnline(bool online, bool wait)
{
    if (wait) {
        QEventLoop loop;
        auto conn = connect(
            AgentManager::self(),
            &AgentManager::instanceOnline,
            this,
            [&](const AgentInstance &instance, bool state) {
                if (instance == *mEwsInstance && state == online) {
                    qDebug() << "is online" << state;
                    loop.exit(0);
                }
            },
            Qt::QueuedConnection);
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, this, [&]() {
            qWarning() << "Timeout waiting for desired resource online state.";
            loop.exit(1);
        });
        timer.start(OnlineStateChangeTimeoutMs);
        mEwsInstance->setIsOnline(online);
        bool status = (loop.exec() == 0);
        disconnect(conn);
        return status;
    } else {
        mEwsInstance->setIsOnline(online);
        return true;
    }
}

OrgKdeAkonadiEwsSettingsInterface &TestAgentInstance::settingsInterface() const
{
    return *mEwsSettingsInterface;
}

OrgKdeAkonadiEwsWalletInterface &TestAgentInstance::walletInterface() const
{
    return *mEwsWalletInterface;
}

OrgKdeAkonadiEwsResourceInterface &TestAgentInstance::resourceInterface() const
{
    return *mEwsResourceInterface;
}

Akonadi::AgentInstance &TestAgentInstance::instance() const
{
    return *mEwsInstance;
}

MsgRootInboxDialogEntry::MsgRootInboxDialogEntry(const QString &rootId, const QString &inboxId, const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    matcher = [](const QDomElement &request) {
        const QDomElement getFolder = getFolderRequest(request, u"IdOnly"_s);
        if (getFolder.isNull()) {
            return false;
        }
        const auto properties = additionalProperties(request);
        const auto fieldUris = childElements(properties, u"FieldURI"_s);
        if (std::none_of(fieldUris.cbegin(), fieldUris.cend(), [](const QDomElement &fieldUri) {
                return fieldUri.attribute(u"FieldURI"_s) == u"folder:DisplayName"_s;
            })) {
            return false;
        }
        return asksForTagProperties(request) && asksForDistinguishedFolders(getFolder, {u"msgfolderroot"_s, u"inbox"_s});
    };
    response = responseEnvelopeStart + QStringLiteral("<m:GetFolderResponse><m:ResponseMessages>") + getFolderResponseMessage(rootId)
        + getFolderResponseMessage(inboxId) + QStringLiteral("</m:ResponseMessages></m:GetFolderResponse>") + responseEnvelopeEnd;
    description = QStringLiteral("GetFolder request for inbox and msgroot");
}

SubscribedFoldersDialogEntry::SubscribedFoldersDialogEntry(const IsolatedTestBase::FolderList &list, const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    static const QList<IsolatedTestBase::Folder::DistinguishedType> specialFolders = {
        IsolatedTestBase::Folder::Inbox,
        IsolatedTestBase::Folder::Calendar,
        IsolatedTestBase::Folder::Tasks,
        IsolatedTestBase::Folder::Contacts,
    };
    QHash<IsolatedTestBase::Folder::DistinguishedType, const IsolatedTestBase::Folder *> folderHash;
    for (const auto &folder : list) {
        if (specialFolders.contains(folder.type)) {
            folderHash.insert(folder.type, &folder);
        }
    }

    QString xml;
    for (auto special : specialFolders) {
        const IsolatedTestBase::Folder *folder = folderHash[special];
        if (QTest::qVerify(folder != nullptr, "folder != nullptr", "", __FILE__, __LINE__)) {
            xml += getFolderResponseMessage(folder->id);
        }
    }

    matcher = [](const QDomElement &request) {
        const QDomElement getFolder = getFolderRequest(request, u"IdOnly"_s);
        if (getFolder.isNull() || !additionalProperties(request).isNull()) {
            return false;
        }
        return asksForDistinguishedFolders(getFolder, {u"inbox"_s, u"calendar"_s, u"tasks"_s, u"contacts"_s});
    };
    response = responseEnvelopeStart + QStringLiteral("<m:GetFolderResponse><m:ResponseMessages>") + xml
        + QStringLiteral("</m:ResponseMessages></m:GetFolderResponse>") + responseEnvelopeEnd;
}

SpecialFoldersDialogEntry::SpecialFoldersDialogEntry(const IsolatedTestBase::FolderList &list, const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    static const QList<IsolatedTestBase::Folder::DistinguishedType> specialFolders = {IsolatedTestBase::Folder::Inbox,
                                                                                      IsolatedTestBase::Folder::Outbox,
                                                                                      IsolatedTestBase::Folder::Sent,
                                                                                      IsolatedTestBase::Folder::Trash,
                                                                                      IsolatedTestBase::Folder::Drafts};
    QHash<IsolatedTestBase::Folder::DistinguishedType, const IsolatedTestBase::Folder *> folderHash;
    for (const auto &folder : list) {
        if (specialFolders.contains(folder.type)) {
            folderHash.insert(folder.type, &folder);
        }
    }

    QString xml;
    for (auto special : specialFolders) {
        const IsolatedTestBase::Folder *folder = folderHash[special];
        if (QTest::qVerify(folder != nullptr, "folder != nullptr", "", __FILE__, __LINE__)) {
            xml += getFolderResponseMessage(folder->id);
        }
    }

    matcher = [](const QDomElement &request) {
        const QDomElement getFolder = getFolderRequest(request, u"IdOnly"_s);
        if (getFolder.isNull() || !additionalProperties(request).isNull()) {
            return false;
        }
        return asksForDistinguishedFolders(getFolder, {u"inbox"_s, u"outbox"_s, u"sentitems"_s, u"deleteditems"_s, u"drafts"_s});
    };
    response = responseEnvelopeStart + QStringLiteral("<m:GetFolderResponse><m:ResponseMessages>") + xml
        + QStringLiteral("</m:ResponseMessages></m:GetFolderResponse>") + responseEnvelopeEnd;
}

GetTagsEmptyDialogEntry::GetTagsEmptyDialogEntry(const QString &rootId, const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    matcher = [](const QDomElement &request) {
        const QDomElement getFolder = getFolderRequest(request, u"IdOnly"_s);
        return !getFolder.isNull() && asksForTagProperties(request) && asksForDistinguishedFolders(getFolder, {u"msgfolderroot"_s});
    };
    response = responseEnvelopeStart + QStringLiteral("<m:GetFolderResponse><m:ResponseMessages>") + getFolderResponseMessage(rootId)
        + QStringLiteral("</m:ResponseMessages></m:GetFolderResponse>") + responseEnvelopeEnd;
}

SubscribeStreamingDialogEntry::SubscribeStreamingDialogEntry(const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    matcher = [](const QDomElement &request) {
        const QDomElement streaming = firstChild(firstChild(requestBody(request), u"Subscribe"_s), u"StreamingSubscriptionRequest"_s);
        if (streaming.isNull()) {
            return false;
        }
        const auto folderIds = childElements(firstChild(streaming, u"FolderIds"_s), u"FolderId"_s);
        if (folderIds.size() != 4) {
            return false;
        }
        QStringList ids;
        ids.reserve(folderIds.size());
        for (const auto &folderId : folderIds) {
            ids.append(folderId.attribute(u"Id"_s));
        }
        for (const auto &expected : {u"aW5ib3g="_s, u"Y2FsZW5kYXI="_s, u"dGFza3M="_s, u"Y29udGFjdHM="_s}) {
            if (!ids.contains(expected)) {
                return false;
            }
        }
        return childElements(firstChild(streaming, u"EventTypes"_s), u"EventType"_s).size() == 6;
    };
    response = responseEnvelopeStart
        + QStringLiteral(
                   "<m:SubscribeResponse>"
                   "<m:ResponseMessages>"
                   "<m:SubscribeResponseMessage ResponseClass=\"Success\">"
                   "<m:ResponseCode>NoError</m:ResponseCode>"
                   "<m:SubscriptionId>otPO1iOPzS+vL5HOuTZY+cCfIIn+vRqOO0ZHTEWAa8k=</m:SubscriptionId>"
                   "</m:SubscribeResponseMessage>"
                   "</m:ResponseMessages>"
                   "</m:SubscribeResponse>")
        + responseEnvelopeEnd;
}

SyncFolderHierInitialDialogEntry::SyncFolderHierInitialDialogEntry(const IsolatedTestBase::FolderList &list,
                                                                   const QString &syncState,
                                                                   const QString &descr,
                                                                   const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    QHash<QString, int> childCount;
    for (const auto &folder : list) {
        ++childCount[folder.parentId];
    }

    QString xml;
    for (const auto &folder : list) {
        if (folder.type == IsolatedTestBase::Folder::Root) {
            continue;
        }
        xml += QStringLiteral("<t:Create>");
        xml += QStringLiteral("<t:Folder>");
        xml += QStringLiteral("<t:FolderId Id=\"%1\" ChangeKey=\"MDAx\" />").arg(folder.id);
        xml += QStringLiteral("<t:ParentFolderId Id=\"%1\" ChangeKey=\"MDAx\" />").arg(folder.parentId);
        QString folderClass;
        QString extraXml;
        switch (folder.type) {
        case IsolatedTestBase::Folder::Calendar:
            folderClass = QStringLiteral("IPF.Calendar");
            break;
        case IsolatedTestBase::Folder::Contacts:
            folderClass = QStringLiteral("IPF.Contacts");
            break;
        case IsolatedTestBase::Folder::Tasks:
            folderClass = QStringLiteral("IPF.Tasks");
            break;
        default:
            folderClass = QStringLiteral("IPF.Note");
            extraXml = QStringLiteral("<t:UnreadCount>0</t:UnreadCount>");
        }
        xml += QStringLiteral("<t:FolderClass>%1</t:FolderClass>").arg(folderClass);
        xml += QStringLiteral("<t:TotalCount>0</t:TotalCount>");
        xml += QStringLiteral("<t:DisplayName>%1</t:DisplayName>").arg(folder.name);
        xml += QStringLiteral("<t:ChildFolderCount>%1</t:ChildFolderCount>").arg(childCount[folder.id]);
        xml += extraXml;
        xml += QStringLiteral("</t:Folder>");
        xml += QStringLiteral("</t:Create>");
    }

    matcher = [](const QDomElement &request) {
        const QDomElement sync = firstChild(requestBody(request), u"SyncFolderHierarchy"_s);
        if (sync.isNull() || elementText(firstChild(firstChild(sync, u"FolderShape"_s), u"BaseShape"_s)) != u"Default"_s) {
            return false;
        }
        const auto properties = additionalProperties(request);
        const auto extendedUris = childElements(properties, u"ExtendedFieldURI"_s);
        if (extendedUris.size() != 1 || !hasAttribute(extendedUris.at(0), u"PropertyTag"_s, u"0x3613"_s)
            || !hasAttribute(extendedUris.at(0), u"PropertyType"_s, u"String"_s)) {
            return false;
        }
        QStringList fieldUris;
        for (const auto &fieldUri : childElements(properties, u"FieldURI"_s)) {
            fieldUris.append(fieldUri.attribute(u"FieldURI"_s));
        }
        if (fieldUris.size() != 2 || !fieldUris.contains(u"folder:EffectiveRights"_s) || !fieldUris.contains(u"folder:ParentFolderId"_s)) {
            return false;
        }
        const QDomElement syncFolderId = firstChild(sync, u"SyncFolderId"_s);
        const auto distinguished = childElements(syncFolderId, u"DistinguishedFolderId"_s);
        return distinguished.size() == 1 && childElements(syncFolderId, u"FolderId"_s).isEmpty()
            && distinguished.at(0).attribute(u"Id"_s) == u"msgfolderroot"_s;
    };
    response = responseEnvelopeStart
        + QStringLiteral(
              "<m:SyncFolderHierarchyResponse><m:ResponseMessages>"
              "<m:SyncFolderHierarchyResponseMessage ResponseClass=\"Success\">"
              "<m:ResponseCode>NoError</m:ResponseCode>"
              "<m:SyncState>%1</m:SyncState>"
              "<m:IncludesLastFolderInRange>true</m:IncludesLastFolderInRange>"
              "<m:Changes>%2</m:Changes>"
              "</m:SyncFolderHierarchyResponseMessage>"
              "</m:ResponseMessages></m:SyncFolderHierarchyResponse>")
              .arg(syncState, xml)
        + responseEnvelopeEnd;
}

/* The resource synchronises the tags of the items of a folder right after the folder
 * hierarchy, so a test that lets the resource come online needs an answer for it. */
SyncFolderItemsDialogEntry::SyncFolderItemsDialogEntry(const QString &syncState, const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    matcher = [](const QDomElement &request) {
        const QDomElement syncItems = firstChild(requestBody(request), u"SyncFolderItems"_s);
        if (syncItems.isNull()) {
            return false;
        }
        const auto extendedUris = childElements(additionalProperties(request), u"ExtendedFieldURI"_s);
        return std::any_of(extendedUris.cbegin(), extendedUris.cend(), [](const QDomElement &extendedUri) {
            return hasAttribute(extendedUri, u"PropertyName"_s, u"Tags"_s);
        });
    };
    response = responseEnvelopeStart
        + QStringLiteral(
              "<m:SyncFolderItemsResponse><m:ResponseMessages>"
              "<m:SyncFolderItemsResponseMessage ResponseClass=\"Success\">"
              "<m:ResponseCode>NoError</m:ResponseCode>"
              "<m:SyncState>%1</m:SyncState>"
              "<m:IncludesLastItemInRange>true</m:IncludesLastItemInRange>"
              "<m:Changes />"
              "</m:SyncFolderItemsResponseMessage>"
              "</m:ResponseMessages></m:SyncFolderItemsResponse>")
              .arg(syncState)
        + responseEnvelopeEnd;
}

UnsubscribeDialogEntry::UnsubscribeDialogEntry(const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    static const auto subscriptionId = u"otPO1iOPzS+vL5HOuTZY+cCfIIn+vRqOO0ZHTEWAa8k="_s;

    matcher = [](const QDomElement &request) {
        const QDomElement unsubscribe = firstChild(requestBody(request), u"Unsubscribe"_s);
        return elementText(firstChild(unsubscribe, u"SubscriptionId"_s)) == subscriptionId;
    };
    response = responseEnvelopeStart
        + QStringLiteral(
                   "<m:UnsubscribeResponse>"
                   "<m:ResponseMessages>"
                   "<m:UnsubscribeResponseMessage ResponseClass=\"Success\">"
                   "<m:ResponseCode>NoError</m:ResponseCode>"
                   "</m:UnsubscribeResponseMessage>"
                   "</m:ResponseMessages>"
                   "</m:UnsubscribeResponse>")
        + responseEnvelopeEnd;
}

ValidateFolderIdsDialogEntry::ValidateFolderIdsDialogEntry(const QStringList &ids, const QString &descr, const ReplyCallback &callback)
    : DialogEntryBase(descr, callback)
{
    QString responseXml;
    for (const auto &folderId : ids) {
        responseXml += getFolderResponseMessage(folderId);
    }

    matcher = [ids](const QDomElement &request) {
        const QDomElement getFolder = getFolderRequest(request, u"IdOnly"_s);
        if (getFolder.isNull() || !additionalProperties(request).isNull()) {
            return false;
        }
        const QDomElement folderIds = firstChild(getFolder, u"FolderIds"_s);
        const auto plainIds = childElements(folderIds, u"FolderId"_s);
        if (!childElements(folderIds, u"DistinguishedFolderId"_s).isEmpty() || plainIds.size() != ids.size()) {
            return false;
        }
        for (qsizetype i = 0; i < ids.size(); ++i) {
            if (plainIds.at(i).attribute(u"Id"_s) != ids.at(i)) {
                return false;
            }
        }
        return true;
    };
    response = responseEnvelopeStart + QStringLiteral("<m:GetFolderResponse><m:ResponseMessages>") + responseXml
        + QStringLiteral("</m:ResponseMessages></m:GetFolderResponse>") + responseEnvelopeEnd;
}

#include "moc_isolatedtestbase.cpp"
