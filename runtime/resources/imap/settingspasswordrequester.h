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

#ifndef SETTINGSPASSWORDREQUESTER_H
#define SETTINGSPASSWORDREQUESTER_H

#include <passwordrequesterinterface.h>

class ImapResource;

class SettingsPasswordRequester : public PasswordRequesterInterface
{
  Q_OBJECT

public:
  SettingsPasswordRequester( ImapResource *resource, QObject *parent = 0 );

  virtual void requestPassword( RequestType request = StandardRequest,
                                const QString &serverError = QString() );

private slots:
  void askUserInput( const QString &serverError );
  void onPasswordRequestCompleted( const QString &password, bool userRejected );

private:
  ImapResource *m_resource;
};

#endif
