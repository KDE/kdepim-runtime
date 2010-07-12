/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef NEPOMUKFEEDERAGENT_H
#define NEPOMUKFEEDERAGENT_H

#include "nepomukfeederagentbase.h"

#include "nie.h"
#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entityhiddenattribute.h>
#include <KDE/KUrl>
#include <Soprano/Vocabulary/NAO>

/** Template part of the shared base class, split out of NepomukFeederAgentBase since moc can't handle templates. */
template <typename CollectionResource>
class NepomukFeederAgent : public NepomukFeederAgentBase
{
  public:
    NepomukFeederAgent(const QString& id) : NepomukFeederAgentBase( id ) {}

    void updateCollection(const Akonadi::Collection& collection, const QUrl& graphUri)
    {
      CollectionResource r( collection.url(), graphUri );
      if ( collection.hasAttribute<Akonadi::EntityHiddenAttribute>() )
        return;

      const Akonadi::EntityDisplayAttribute *attr = collection.attribute<Akonadi::EntityDisplayAttribute>();

      if ( attr && !attr->displayName().isEmpty() )
        r.setLabel( attr->displayName() );
      else
        r.setLabel( collection.name() );
      if ( attr && !attr->iconName().isEmpty() )
        r.addProperty( Soprano::Vocabulary::NAO::hasSymbol(), Soprano::LiteralValue( attr->iconName() ) );
      setParent( r, collection );
    }

    void itemMoved(const Akonadi::Item& item, const Akonadi::Collection& collectionSource, const Akonadi::Collection& collectionDestination)
    {
      entityMoved( item, collectionSource, collectionDestination );
    }

    void collectionMoved(const Akonadi::Collection& collection, const Akonadi::Collection& source, const Akonadi::Collection& destination)
    {
      entityMoved( collection, source, destination );
    }

  private:
    template <typename E>
    void entityMoved( const E &entity, const Akonadi::Collection &source, const Akonadi::Collection &destination )
    {
      Soprano::Model *model = Nepomuk::ResourceManager::instance()->mainModel();
      model->removeStatement( Soprano::Statement( entity.url(), Vocabulary::NIE::isPartOf(), source.url() ) );
      model->addStatement( Soprano::Statement( entity.url(), Vocabulary::NIE::isPartOf(), destination.url() ) );
    }
};

#endif
