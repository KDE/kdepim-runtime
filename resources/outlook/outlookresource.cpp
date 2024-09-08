#include "outlookresource.h"
#include "resource_state.cxxqt.h"

OutlookResource::OutlookResource(const QString &id)
    : Akonadi::ResourceBase(id)
    , mState(std::make_unique<ResourceState>())
{
    connect(mState.get(), &ResourceState::collectionsRetrieved, 
            [this](const auto &collections) {
                collectionsRetrieved(collections);
            });
}

OutlookResource::~OutlookResource()
{
}

void OutlookResource::configure(WId windowId)
{
    Q_UNUSED(windowId);
}

void OutlookResource::retrieveCollections()
{
    mState->retrieveCollections();
}

void OutlookResource::retrieveItems(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
}

void OutlookResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(item);
    Q_UNUSED(parts);
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

void OutlookResource::collectionAdded(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
}

void OutlookResource::collectionChanged(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
}

void OutlookResource::collectionRemoved(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
}

AKONADI_RESOURCE_MAIN(OutlookResource)

#include "outlookresource.moc"
