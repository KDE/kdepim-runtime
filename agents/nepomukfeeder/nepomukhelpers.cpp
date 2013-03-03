/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nepomukhelpers.h"

#include <nepomuk2/simpleresource.h>
#include <nepomuk2/simpleresourcegraph.h>
#include <nepomuk2/datamanagement.h>
#include <nepomuk2/storeresourcesjob.h>
#include <nepomuk2/resourcemanager.h>
#include <nepomuk2/resource.h>
#include <nepomuk2/variant.h>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Model>
#include <soprano/queryresultiterator.h>
#include <Nepomuk2/Vocabulary/NIE>
#include <aneo.h>

#include <Akonadi/Item>
#include <Akonadi/Collection>
#include <Akonadi/EntityDisplayAttribute>

#include <KJob>

#include <QDateTime>

#include <nepomukfeederplugin.h>
#include <nepomukfeederutils.h>

#include "pluginloader.h"
#include "nepomukfeeder-config.h"

using namespace Nepomuk2::Vocabulary;

namespace  NepomukHelpers {

/** Set the parent collection of the entity @param entity */
void setParentCollection( const Akonadi::Entity &entity, Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph )
{
  if ( entity.parentCollection().isValid() && entity.parentCollection() != Akonadi::Collection::root() ) {
    Nepomuk2::SimpleResource parentResource( entity.parentCollection().url() );
    parentResource.setTypes( QList <QUrl>() << Vocabulary::ANEO::AkonadiDataObject() << NIE::InformationElement() );
    graph << parentResource; //To use the nie::isPartOf relation both parent and child must be in the graph
    res.setProperty( NIE::isPartOf(), parentResource );
  }
}


KJob *addCollectionToNepomuk( const Akonadi::Collection &collection)
{
  //kDebug() << collection.url();
  Nepomuk2::SimpleResourceGraph graph;
  Nepomuk2::SimpleResource res;
  res.setTypes( QList <QUrl>() << Vocabulary::ANEO::AkonadiDataObject() << NIE::InformationElement() );
  res.setProperty( NIE::url(), collection.url() );
  setParentCollection( collection, res, graph );

  const Akonadi::EntityDisplayAttribute *attr = collection.attribute<Akonadi::EntityDisplayAttribute>();

  if ( attr && !attr->displayName().isEmpty() ) {
      res.setProperty( Soprano::Vocabulary::NAO::prefLabel(), attr->displayName() );
  } else {
      res.setProperty( Soprano::Vocabulary::NAO::prefLabel(), collection.name() );
  }
  if ( attr && !attr->iconName().isEmpty() )
      NepomukFeederUtils::setIcon( attr->iconName(), res, graph );

  foreach ( const QString &type, collection.contentMimeTypes() ) {
      //QSharedPointer<Akonadi::NepomukFeederPlugin> plugin = FeederPluginloader::instance().feederPluginForMimeType( type );
      foreach ( const QSharedPointer<Akonadi::NepomukFeederPlugin> &plugin , FeederPluginloader::instance().feederPluginsForMimeType( type ) ) {
        plugin->updateCollection( collection, res, graph ); //FIXME create one resource for each feeder, nepomuk will merge if possible
      }
  }

  graph.insert( res );
  /*kDebug() << "--------------------------------";
  foreach ( const Nepomuk::SimpleResource &res, graph.toList() ) {
      kDebug() << res.property( Soprano::Vocabulary::RDF::type() );
  }
  kDebug() << "--------------------------------";*/
  QHash <QUrl, QVariant> additionalMetadata;
  additionalMetadata.insert( Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::DiscardableInstanceBase() );
  //We overwrite properties, as the resource is not removed on update, and there are not subproperties on collections (if there were, we would remove them first too)
  return Nepomuk2::storeResources( graph, Nepomuk2::IdentifyNew, Nepomuk2::OverwriteProperties, additionalMetadata, KGlobal::mainComponent() );
}

KJob *markCollectionAsIndexed( const Akonadi::Collection &collection )
{
    kDebug() << "marking collection as completed: " << collection.id();
    Nepomuk2::SimpleResourceGraph graph;
    Nepomuk2::SimpleResource res;
    res.setProperty( NIE::url(), collection.url() );
    res.setProperty( Vocabulary::ANEO::akonadiIndexCompatLevel(), NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL );
    graph << res;
    QHash <QUrl, QVariant> additionalMetadata;
    additionalMetadata.insert( Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::DiscardableInstanceBase() );
    return Nepomuk2::storeResources( graph, Nepomuk2::IdentifyNew, Nepomuk2::OverwriteProperties, additionalMetadata, KGlobal::mainComponent() );
}

void addItemToGraph( const Akonadi::Item &item, Nepomuk2::SimpleResourceGraph &graph )
{
  //kDebug() << item.url();
  Nepomuk2::SimpleResource res;
  res.setTypes( QList <QUrl>() << Vocabulary::ANEO::AkonadiDataObject() << NIE::InformationElement() );
  res.setProperty( NIE::url(), QUrl( item.url() ) );
  res.setProperty( NIE::lastModified(), item.modificationTime() );
  res.setProperty( Vocabulary::ANEO::akonadiIndexCompatLevel(), NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL );
  Q_ASSERT( res.property( NIE::url() ).first().toUrl() == QUrl( item.url() ) );
  res.setProperty( Vocabulary::ANEO::akonadiItemId(), QString::number( item.id() ) );
  setParentCollection( item, res, graph );
  foreach ( const QSharedPointer<Akonadi::NepomukFeederPlugin> &plugin , FeederPluginloader::instance().feederPluginsForMimeType( item.mimeType() ) ) {
    plugin->updateItem( item, res, graph );
  }
  graph << res;
}

/** Saves the graph, and marks the data as discardable. Use this function to store data created by the feeder */
KJob *addGraphToNepomuk( const Nepomuk2::SimpleResourceGraph &graph )
{
  /*kDebug() << "--------------------------------";
  foreach ( const Nepomuk2::SimpleResource &res, graph.toList() ) {
    if ( res.contains( NIE::url() ) ) {
        Q_ASSERT( res.property( NIE::url() ).size() == 1 );
        kDebug() << res.property( NIE::url() ).first().toUrl();
        kDebug() << res.property( Soprano::Vocabulary::NAO::prefLabel() );
    }
  }
  kDebug() << "--------------------------------";*/
  QHash <QUrl, QVariant> additionalMetadata;
  additionalMetadata.insert( Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::DiscardableInstanceBase() );
  return Nepomuk2::storeResources( graph, Nepomuk2::IdentifyNew, Nepomuk2::NoStoreResourcesFlags, additionalMetadata, KGlobal::mainComponent() );
}

bool isIndexed(const Akonadi::Item& item)
{
  // we check if the item already exists with the following values:
  // - nie:url needs to be set
  // - aneo:akonadiItemId needs to be set
  // - nie:lastModified needs to match the item's modification time
  // - aneo:akonadiIndexCompatLevel needs to match the indexer's level
  return Nepomuk2::ResourceManager::instance()->mainModel()->executeQuery( QString::fromLatin1( "ask where { ?r %1 %2 ; %3 %4 ; %5 %6 ; %7 %8 . }" )
                                                                    .arg( Soprano::Node::resourceToN3( NIE::url() ),
                                                                            Soprano::Node::resourceToN3( item.url() ),
                                                                            Soprano::Node::resourceToN3( Vocabulary::ANEO::akonadiItemId() ),
                                                                            Soprano::Node::literalToN3( QString::number( item.id() ) ),
                                                                            Soprano::Node::resourceToN3( NIE::lastModified() ),
                                                                            Soprano::Node::literalToN3( item.modificationTime() ),
                                                                            Soprano::Node::resourceToN3( Vocabulary::ANEO::akonadiIndexCompatLevel() ),
                                                                            Soprano::Node::literalToN3( NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL ) ),
                                                                            Soprano::Query::QueryLanguageSparql ).boolValue();
}

bool isIndexed(const Akonadi::Collection& collection)
{
  // we check if the collection already has been indexed with the following values
  // - nie:url needs to be set
  // - aneo:akonadiIndexCompatLevel needs to match the indexer's level
  return Nepomuk2::ResourceManager::instance()->mainModel()->executeQuery( QString::fromLatin1( "ask where { ?r %1 %2 ; %3 %4 . }" )
                                                                    .arg( Soprano::Node::resourceToN3( NIE::url() ),
                                                                            Soprano::Node::resourceToN3( collection.url() ),
                                                                            Soprano::Node::resourceToN3( Vocabulary::ANEO::akonadiIndexCompatLevel() ),
                                                                            Soprano::Node::literalToN3( NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL ) ),
                                                                            Soprano::Query::QueryLanguageSparql ).boolValue();
}


}
