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

#ifndef OXA_USERS_H
#define OXA_USERS_H

#include "user.h"

#include <QtCore/QMap>
#include <QtCore/QObject>


namespace OXA {

class Users : public QObject
{
  Q_OBJECT

  public:
    ~Users();

    static Users* self();

    void init( const QString &identifier );

    qlonglong currentUserId() const;

    User lookupUid( qlonglong uid ) const;
    User lookupEmail( const QString &email ) const;

    QString cacheFilePath() const;

  private:
    friend class UpdateUsersJob;

    Users();
    void setCurrentUserId( qlonglong );
    void setUsers( const User::List& );

    void loadFromCache();
    void saveToCache();

    qlonglong mCurrentUserId;
    QMap<qlonglong, User> mUsers;
    QString mIdentifier;

    static Users* mSelf;
};

}

#endif
