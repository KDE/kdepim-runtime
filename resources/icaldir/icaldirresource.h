/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef ICALDIRRESOURCE_H
#define ICALDIRRESOURCE_H

#include "../shared/dirresource.h"

#include <KCalCore/Incidence>

class ICalDirResource : public DirResource<KCalCore::Incidence::Ptr>
{
  public:
    explicit ICalDirResource( const QString &id );
    ~ICalDirResource();

  protected:
    void retrieveCollections();

    QString payloadId( const KCalCore::Incidence::Ptr &payload ) const;
    QString mimeType() const;
    bool isEmpty( const KCalCore::Incidence::Ptr &payload ) const {
        return payload.isNull();
    }

    KCalCore::Incidence::Ptr readFromFile( const QString &file ) const;
    bool writeToFile( const KCalCore::Incidence::Ptr &payload ) const;

};

#endif
