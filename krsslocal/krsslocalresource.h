#ifndef KRSSLOCALRESOURCE_H
#define KRSSLOCALRESOURCE_H

#include <Akonadi/CachePolicy>
#include <Akonadi/ResourceBase>
#include <boost/shared_ptr.hpp>
#include <krssresource/opmlparser.h>
#include <Syndication/Syndication>
#include <QTimer>

class KRssLocalResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KRssLocalResource( const QString &id );
    ~KRssLocalResource();
    Akonadi::Collection::List buildCollectionTree( QList<boost::shared_ptr<const KRssResource::ParsedNode> > listOfNodes, 
 				   Akonadi::Collection::List &list, Akonadi::Collection &parent);
    QString mimeType();
    void writeFeedsToOpml(const QString &path, const QList<Akonadi::Collection>& feeds);

  public Q_SLOTS:
    virtual void configure( WId windowId );    
  signals:
    void quitOrTimeout();

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void slotLoadingComplete(Syndication::Loader* loader, Syndication::FeedPtr feed, 
			Syndication::ErrorCode status );
    void fetchCollections();
    void fetchCollectionsFinished( KJob *job );

			
  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    
  private:    
    Akonadi::CachePolicy policy;
    //QList<Syndication::ItemPtr> m_syndItems;
    QTimer *writeBackTimer;
    static const int CACHE_TIMEOUT = -1, INTERVAL_CHECK_TIME = 5; 
    static const int WRITE_BACK_TIMEOUT = 30000; // in milliseconds
    
};

#endif
