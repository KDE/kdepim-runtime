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

#ifndef AKONADI_NEPOMUKFEEDERPLUGIN_H
#define AKONADI_NEPOMUKFEEDERPLUGIN_H

#include <QObject>

#include <dms-copy/simpleresource.h>
#include <dms-copy/simpleresourcegraph.h>

#include <akonadi/item.h>
#include <akonadi/collection.h>

namespace Akonadi {

class NepomukFeederPlugin: public QObject
{
  Q_OBJECT
  public:
    explicit NepomukFeederPlugin(QObject* parent = 0): QObject(parent){};
    virtual ~NepomukFeederPlugin() {}
    /** Reimplement to do the actual work. 
     *  
     * It is only necessary to add the attributes to @param res,
     * the storing of the resource will happen automatically.
     * If additional resources are needed, the can be added to @param graph.
     * Additionaly created resources are only removed before an update if they are subresources of the @param res.
     * 
     * It is not necessary for the reimplementation to add @param res to @param graph, nor to store @param graph.
     */
    virtual void updateItem( const Akonadi::Item &item, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph ) = 0;
     /**
     * Sets the label and icon from the EntityDisplayAttribute.
     *
     * Collections are not supposed to have subresources, so they would not be removed on an update.
     */
    virtual void updateCollection( const Akonadi::Collection &collection, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph ){};
};

}

Q_DECLARE_INTERFACE( Akonadi::NepomukFeederPlugin, "org.freedesktop.Akonadi.NepomukFeederPlugin/1.0" )

#endif
