#ifndef KRSSLOCALRESOURCE_H
#define KRSSLOCALRESOURCE_H

#include <akonadi/resourcebase.h>

class krsslocalResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    krsslocalResource( const QString &id );
    ~krsslocalResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    
};

#endif
