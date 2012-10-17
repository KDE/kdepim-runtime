/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AKONADI_NEPOMUKFEEDERPLUGIN_H
#define AKONADI_NEPOMUKFEEDERPLUGIN_H

#include <QObject>

#include <nepomuk2/simpleresource.h>
#include <nepomuk2/simpleresourcegraph.h>

#include <akonadi/item.h>
#include <akonadi/collection.h>

namespace Akonadi {

class NepomukFeederPlugin: public QObject
{
  Q_OBJECT
  public:
    explicit NepomukFeederPlugin(QObject* parent = 0): QObject(parent){}
    virtual ~NepomukFeederPlugin() {}
    /** Reimplement to do the actual work. 
     *  
     * It is only necessary to add the attributes to @param res,
     * the storing of the resource will happen automatically.
     * If additional resources are needed, the can be added to @param graph.
     * Additionaly created resources are only removed on removal of the item if they are subresources of the @param res.
     * Properties are not removed on an update, but only overwritten. In case you need to remove properties after an update,
     * you have to remove the affected properties yourself during each update.
     * 
     * It is not necessary for the reimplementation to add @param res to @param graph, nor to store @param graph.
     */
    virtual void updateItem( const Akonadi::Item &item, Nepomuk2::SimpleResource &res, Nepomuk2::SimpleResourceGraph &graph ) = 0;
     /**
     * Sets the label and icon from the EntityDisplayAttribute.
     *
     * Collections are not supposed to have subresources, so they would not be removed on an update.
     */
    virtual void updateCollection( const Akonadi::Collection &collection, Nepomuk2::SimpleResource &res, Nepomuk2::SimpleResourceGraph &graph ){ Q_UNUSED( collection ); Q_UNUSED( res ); Q_UNUSED( graph ) }
};

}

Q_DECLARE_INTERFACE( Akonadi::NepomukFeederPlugin, "org.freedesktop.Akonadi.NepomukFeederPlugin/1.0" )

#endif
