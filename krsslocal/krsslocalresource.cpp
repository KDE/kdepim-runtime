#include "krsslocalresource.h"

#include "settings.h"
#include "settingsadaptor.h"

#include <QtDBus/QDBusConnection>

#include <KFileDialog>
#include <KDebug>
#include <KStandardDirs>
#include <KLocale>

using namespace Akonadi;

krsslocalResource::krsslocalResource( const QString &id )
  : ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  // TODO: you can put any resource specific initialization code here.
}

krsslocalResource::~krsslocalResource()
{
}

void krsslocalResource::retrieveCollections()
{
  // TODO: this method is called when Akonadi wants to have all the
  // collections your resource provides.
  // Be sure to set the remote ID and the content MIME types
}

void krsslocalResource::retrieveItems( const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );

  // TODO: this method is called when Akonadi wants to know about all the
  // items in the given collection. You can but don't have to provide all the
  // data for each item, remote ID and MIME type are enough at this stage.
  // Depending on how your resource accesses the data, there are several
  // different ways to tell Akonadi when you are done.
}

bool krsslocalResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );

  // TODO: this method is called when Akonadi wants more data for a given item.
  // You can only provide the parts that have been requested but you are allowed
  // to provide all in one go

  return true;
}

void krsslocalResource::aboutToQuit()
{
  // TODO: any cleanup you need to do while there is still an active
  // event loop. The resource will terminate after this method returns
}

void krsslocalResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  const QString oldPath = Settings::self()->path();
  
  KUrl startUrl;
  if ( oldPath.isEmpty() )
    startUrl = KUrl( QDir::homePath() );
  else
    startUrl = KUrl( oldPath );
  
  const QString title = i18nc("@title:window", "Select an OPML Document");
  KUrl newPath = KFileDialog::getOpenUrl( startUrl, QLatin1String("*.opml|") + i18n("OPML Document (*.opml)"),
                                              0, title );
  
  if ( newPath.isEmpty() )
    newPath = KStandardDirs::locateLocal( "appdata", QLatin1String("feeds.opml") );
    
  Settings::self()->setPath( newPath.url() );
  Settings::self()->writeConfig();
  synchronize();
  
}

void krsslocalResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  Q_UNUSED( item );
  Q_UNUSED( collection );

  // TODO: this method is called when somebody else, e.g. a client application,
  // has created an item in a collection managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple
}

void krsslocalResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );

  // TODO: this method is called when somebody else, e.g. a client application,
  // has changed an item managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple
}

void krsslocalResource::itemRemoved( const Akonadi::Item &item )
{
  Q_UNUSED( item );

  // TODO: this method is called when somebody else, e.g. a client application,
  // has deleted an item managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple
}

AKONADI_RESOURCE_MAIN( krsslocalResource )

#include "krsslocalresource.moc"
