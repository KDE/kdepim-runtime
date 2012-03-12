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
#ifndef NEPOMUKHELPERS_H
#define NEPOMUKHELPERS_H

namespace Nepomuk {
class SimpleResourceGraph;
class SimpleResource;
}

namespace Akonadi {
class Collection;
class Item;
class Entity;
}

class KJob;

namespace  NepomukHelpers {
  
/** Set the parent collection of the entity @param entity */
void setParentCollection( const Akonadi::Entity &entity, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph );

KJob *addCollectionToNepomuk( const Akonadi::Collection &collection);
KJob *markCollectionAsIndexed( const Akonadi::Collection &collection );
void addItemToGraph( const Akonadi::Item &item, Nepomuk::SimpleResourceGraph &graph );

/** Saves the graph, and marks the data as discardable. Use this function to store data created by the feeder */
KJob *addGraphToNepomuk( const Nepomuk::SimpleResourceGraph &graph );


}

#endif
