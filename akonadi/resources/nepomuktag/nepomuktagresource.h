/*
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#ifndef __NEPOMUKTAG_RESOURCE_H__
#define __NEPOMUKTAG_RESOURCE_H__

#include <akonadi/resourcebase.h>

class NepomukTagResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.NepomukTag.Resource" )

public:
    NepomukTagResource( const QString &id );
    ~NepomukTagResource();

public Q_SLOTS:
    void configure( WId windowId );

protected Q_SLOTS:
    virtual void retrieveCollections();
    virtual void retrieveItems( const Akonadi::Collection &col );
    virtual bool retrieveItem( const Akonadi::Item &, const QSet<QByteArray> & ) { return true; };

private Q_SLOTS:
    void slotResult( KJob* );

protected:
    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
};

#endif
