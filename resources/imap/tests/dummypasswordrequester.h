/*
   Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   Author: Kevin Ottens <kevin@kdab.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef DUMMYPASSWORDREQUESTER_H
#define DUMMYPASSWORDREQUESTER_H

#include "passwordrequesterinterface.h"

class DummyPasswordRequester : public PasswordRequesterInterface
{
  Q_OBJECT
public:
  DummyPasswordRequester( QObject *parent = 0 );

  QString password() const;
  void setPassword( const QString &password );

public:
  virtual void requestPassword( RequestType request = StandardRequest,
                                const QString &serverError = QString() );

private slots:
  void emitPassword();

private:
  QString m_password;
  ResultType m_result;
};

#endif

