#include "outlookresource.h"
#include "conversion.h"
#include "cxx-qt-gen/resource_state.cxxqt.h"

OutlookResource::OutlookResource(const QString &id)
    : Akonadi::ResourceBase(id)
    , mState(std::make_unique<ResourceState>())
{
    connect(mState.get(), &ResourceState::collectionsRetrieved, this, [this](const auto &collections) {
        collectionsRetrieved(intoAkonadi(collections));
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
