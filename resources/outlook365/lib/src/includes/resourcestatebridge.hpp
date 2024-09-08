#pragma once

#include <vector>

#include <QObject>

struct Collection;
struct Item;

class ResourceStateBridge : public QObject
{
    Q_OBJECT

public:
    ~ResourceStateBridge() = default;
    void collectionsRetrieved(const std::vector<Collection> &collections);
    void itemsRetrieved(const std::vector<Item> &items);
    void itemsRetrievedIncremental(const std::vector<Item> &items);

    void itemAdded(const Item &item);
    void itemUpdated(const Item &item);
    void itemRemoved();

    void collectionAdded(const Collection &collection);
    void collectionUpdated(const Collection &collection);
    void collectionRemoved();

    void itemProcessed(const Item &item);
    void itemsProcessed(const std::vector<Item> &items);
    void collectionProcessed(const Collection &collection);
    void changeProcessed();
};
