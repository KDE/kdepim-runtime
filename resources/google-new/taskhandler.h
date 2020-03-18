#ifndef TASKHANDLER_H
#define TASKHANDLER_H

#include "generichandler.h"
#include <QObject>


class TaskHandler : public GenericHandler
{
public:
    using GenericHandler::GenericHandler;

    QString mimetype();
    bool canPerformTask(const Akonadi::Item &item) override;

    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override; 
    
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) override;
    void itemRemoved(const Akonadi::Item &item) override;
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;
    
    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;
private Q_SLOTS:
    void slotCollectionsRetrieved(KGAPI2::Job* job);
    void slotItemsRetrieved(KGAPI2::Job* job);
    void slotCreateJobFinished(KGAPI2::Job* job);
    void slotRemoveTaskFetchJobFinished(KJob *job);
    void slotTaskAddedSearchFinished(KJob* job);
    void slotDoRemoveTask(KJob *job);
};

#endif // TASKHANDLER_H
