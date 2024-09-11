#pragma once

#include <Akonadi/ResourceBase>

class ResourceState;

class OutlookResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT
public:
    explicit OutlookResource(const QString &id);
    ~OutlookResource() override;

    void configure(WId windowId) override;

    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

private:
    void resetState();

    std::unique_ptr<ResourceState> mState;
};
