#include "krsslocalresource.h"

#include "settings.h"
#include "settingsadaptor.h"

#include <QtDBus/QDBusConnection>

#include <KFileDialog>
#include <KDebug>
#include <KStandardDirs>
#include <KLocale>
#include <QtXml/QXmlStreamReader>
#include <QMessageBox>
#include <boost/shared_ptr.hpp>
#include <akonadi/entitydisplayattribute.h>

#include <krssresource/opmlparser.h>

using namespace Akonadi;
using namespace KRssResource;
using namespace boost;

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
  
/*   QList<Akonadi::Collection> feeds = fjob->feeds();
   collectionsRetrieved( feeds );
*/     
    const QString path = Settings::self()->path();
    
    /* We'll parse the opml file */
    QFile* file = new QFile( path );
    /* If we can't open it, let's show an error message. */
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
	QMessageBox::critical(0, i18n("krsslocalresource::retrieveCollections"), 
		                    i18n("Couldn't open ") + path, 
		                    QMessageBox::Ok);
	return;
    }

    
    QXmlStreamReader reader( file );
    
    OpmlReader parser;
    
    while ( !reader.atEnd() ) {
        reader.readNext();

        if ( reader.isStartElement() ) {
            if ( reader.name().toString().toLower() == QLatin1String("opml") ) {
                kDebug() << "OPML version" << reader.attributes().value( QLatin1String("version") ).toString();
                parser.readOpml( reader );
            }
            else {
                reader.raiseError( i18n ( "The file is not an valid OPML document." ) );
            }
        }
    }
    
    QList<shared_ptr<const ParsedFeed> > parsedNodes = parser.feeds();
    
    // create a top-level collection
    Collection top;
    top.setParent( Akonadi::Collection::root() );
    top.setRemoteId( Settings::path() );
    top.setName( Settings::path() );
    top.setContentMimeTypes( QStringList( Akonadi::Collection::mimeType() ) );
    //customize icon of the collection
    top.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing )->setIconName( QString("application-rss+xml") );
    
    Collection::List list;
    list << top;
    
    foreach(const shared_ptr<const ParsedFeed>& parsedNode, parsedNodes) {
	Collection c = parsedNode->toAkonadiCollection();
	c.setParent(top);
	list << c;
    }
    
    collectionsRetrieved( list );
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
  QString newPath = KFileDialog::getOpenFileName( startUrl, QLatin1String("*.opml|") + i18n("OPML Document (*.opml)"),
                                              0, title );
  
  if ( newPath.isEmpty() )
    newPath = KStandardDirs::locateLocal( "appdata", QLatin1String("feeds.opml") );
    
  Settings::self()->setPath( newPath );
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
