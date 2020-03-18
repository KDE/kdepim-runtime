#ifndef GENERICHANDLER_H
#define GENERICHANDLER_H

#include <KGAPI/Types>

#include <QSharedPointer>
#include <QObject>

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>

namespace KGAPI2 {
    class Job;
}

class GoogleResource;

class GenericHandler : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<GenericHandler> Ptr;

    GenericHandler(GoogleResource* resource);
    virtual ~GenericHandler();

    virtual QString mimetype() = 0;
    virtual bool canPerformTask(const Akonadi::Item &item) = 0;
    
    virtual void retrieveCollections() = 0;
    virtual void retrieveItems(const Akonadi::Collection &collection) = 0;

    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) = 0;
    virtual void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) = 0;
    virtual void itemRemoved(const Akonadi::Item &item) = 0;
    virtual void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) = 0;

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) = 0;
    virtual void collectionChanged(const Akonadi::Collection &collection) = 0;
    virtual void collectionRemoved(const Akonadi::Collection &collection) = 0;
protected:
    GoogleResource* m_resource;
    void emitReadyStatus();
Q_SIGNALS:
    void status(int code, const QString& message = QString());
    void percent(int progress);
    void collectionsRetrieved(const Akonadi::Collection::List& collections);
};

#endif // GENERICHANDLER_H
