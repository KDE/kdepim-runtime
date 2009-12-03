/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef OXA_OBJECT_H
#define OXA_OBJECT_H

#include <boost/shared_ptr.hpp>
#include <kabc/addressee.h>
#include <kcal/incidence.h>

#include <QtCore/QList>
#include <QtCore/QString>

namespace OXA {

class Object
{
  public:
    typedef QList<Object> List;

    Object();

    enum Type
    {
      Contact,
      Event,
      Task
    };

    void setObjectId( qlonglong id );
    qlonglong objectId() const;

    void setFolderId( qlonglong id );
    qlonglong folderId() const;

    void setLastModified( const QString &timeStamp );
    QString lastModified() const;

    void setType( Type type );
    Type type() const;

    void setContact( const KABC::Addressee &contact );
    KABC::Addressee contact() const;

    void setEvent( const KCal::Incidence::Ptr &event );
    KCal::Incidence::Ptr event() const;

    void setTask( const KCal::Incidence::Ptr &task );
    KCal::Incidence::Ptr task() const;

  private:
    qlonglong mObjectId;
    qlonglong mFolderId;
    QString mLastModified;
    Type mType;
    KABC::Addressee mContact;
    KCal::Incidence::Ptr mEvent;
    KCal::Incidence::Ptr mTask;
};

}

#endif
