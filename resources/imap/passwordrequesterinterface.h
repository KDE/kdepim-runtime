/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef PASSWORDREQUESTERINTERFACE_H
#define PASSWORDREQUESTERINTERFACE_H

#include <QtCore/QObject>

class PasswordRequesterInterface : public QObject
{
  Q_OBJECT
  Q_ENUMS( ResultType RequestType )

public:
  enum ResultType {
    PasswordRetrieved,
    ReconnectNeeded,
    UserRejected,
    EmptyPasswordEntered
  };

  enum RequestType {
    StandardRequest,
    WrongPasswordRequest
  };

protected:
  PasswordRequesterInterface( QObject *parent = 0 );

public:
  virtual void requestPassword( RequestType request = StandardRequest,
                                const QString &serverError = QString() ) = 0;

signals:
  void done( int resultType, const QString &password = QString() );
};

#endif
