/*
    Copyright (c) 2010 Tom Albers <toma@kde.org>

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

#ifndef SERVERTEST_H
#define SERVERTEST_H

#include <QtCore/QObject>

namespace MailTransport {
    class ServerTest;
}

class ServerTest : public QObject
{
  Q_OBJECT
  public:
    explicit ServerTest( QObject *parent );
    ~ServerTest();

  public slots:
    /* @p protocol being 'imap' 'smtp' or 'pop3' */
    Q_SCRIPTABLE void test( const QString server, const QString protocol );

  signals:
    /* returns the advised setting, @p result begin 'ssl' 'tls' or 'none'. */
    void testResult( const QString& result );

    /* returns if no connection is possible, test failed. */
    void testFail();

  private slots:
    void testFinished( QList< int > list );

  private:
    MailTransport::ServerTest* m_serverTest;
};

#endif
