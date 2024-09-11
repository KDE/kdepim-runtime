#include "outlookresource.h"
#include "conversion.h"
#include "cxx-qt-gen/resource_state.cxxqt.h"
#include "outlookresource_debug.h"
#include "outlooksettings.h"

#include <KLocalizedString>

static const QString ROOT_COLLECTION_REMOTE_ID = QStringLiteral("outlook_root");

OutlookResource::OutlookResource(const QString &id)
    : Akonadi::ResourceBase(id)
{
    connect(this, &Akonadi::ResourceBase::reloadConfiguration, this, [this] {
        qCDebug(OUTLOOK_LOG) << "Resource" << identifier() << "reconfigured";
        resetState();
    });

    resetState();
}

OutlookResource::~OutlookResource() = default;

void OutlookResource::resetState()
{
    auto settings = OutlookSettings(config());
    mState = std::make_unique<ResourceState>(settings.accessToken(), settings.refreshToken());
    connect(mState.get(), &ResourceState::collectionsRetrieved, this, [this](const auto &collections) {
        qCDebug(OUTLOOK_LOG) << collections.size() << "collections received";
        Akonadi::Collection parent;
        parent.setName(i18n("Outlook"));
        parent.setRemoteId(ROOT_COLLECTION_REMOTE_ID);
        parent.setParentCollection(Akonadi::Collection::root());
        parent.setContentMimeTypes({Akonadi::Collection::mimeType()});
        parent.setRights(Akonadi::Collection::ReadOnly);
        auto akonadiCollections = intoAkonadi(collections, parent);
        akonadiCollections.push_back(parent);
        collectionsRetrieved(akonadiCollections);
    });
    connect(mState.get(), &ResourceState::taskFailed, this, [this](const QString &error) {
        qCWarning(OUTLOOK_LOG) << "Task failed:" << error;
        cancelTask();
    });
}

void OutlookResource::configure(WId windowId)
{
    Q_UNUSED(windowId);
}

void OutlookResource::retrieveCollections()
{
    qCDebug(OUTLOOK_LOG) << "Retrieving collections";
    mState->retrieveCollections();
}

void OutlookResource::retrieveItems(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
}

bool OutlookResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(item);
    Q_UNUSED(parts);
    return false;
}

void OutlookResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(item);
    Q_UNUSED(collection);
}

void OutlookResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(item);
    Q_UNUSED(parts);
}

void OutlookResource::itemRemoved(const Akonadi::Item &item)
{
    Q_UNUSED(item);
}

void OutlookResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(collection);
    Q_UNUSED(parent);
}

void OutlookResource::collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes)
{
    Q_UNUSED(collection);
    Q_UNUSED(changedAttributes);
}

void OutlookResource::collectionRemoved(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
}

AKONADI_RESOURCE_MAIN(OutlookResource)
