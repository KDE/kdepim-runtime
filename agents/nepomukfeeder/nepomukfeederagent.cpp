/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "nepomukfeederagent.h"

#include <akonadi/item.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>

#include <KUrl>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>

NepomukFeederAgent::NepomukFeederAgent(const QString& id) : AgentBase(id)
{
  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();
}

NepomukFeederAgent::~NepomukFeederAgent()
{
}

void NepomukFeederAgent::removeItemFromNepomuk( const Akonadi::Item &item )
{
  // find the graph that contains our item and delete the complete graph
  QList<Soprano::Node> list = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(
                QString( "select ?g where { ?g <http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor> %1. }")
                       .arg( Soprano::Node::resourceToN3( item.url() ) ),
                Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

  foreach ( const Soprano::Node &node, list )
    Nepomuk::ResourceManager::instance()->mainModel()->removeContext( node );
}


void NepomukFeederAgent::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
  Q_UNUSED( collection );
  if ( item.hasPayload() )
    updateItem( item );
}

void NepomukFeederAgent::itemChanged(const Akonadi::Item& item, const QSet< QByteArray >& partIdentifiers)
{
  // TODO: check part identfiers if anything interesting changed at all
  if ( item.hasPayload() )
    updateItem( item );
}

void NepomukFeederAgent::itemRemoved(const Akonadi::Item& item)
{
  removeItemFromNepomuk( item );
}

#include "nepomukfeederagent.moc"
