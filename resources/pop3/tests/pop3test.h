/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef POP3TEST_H
#define POP3TEST_H

#include "fakeserver/fakeserver.h"
#include "settings.h"

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <QtCore/QObject>
#include <QtCore/QList>

class Pop3Test : public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSimpleDownload();
    void testSimpleLeaveOnServer();
    void testBigFetch();

  private:
    void cleanupMaildir( Akonadi::Item::List items );
    void checkMailsInMaildir( const QList< QByteArray >& mails );
    Akonadi::Item::List checkMailsOnAkonadiServer( const QList<QByteArray> &mails );
    void syncAndWaitForFinish();
    QString loginSequence() const;
    QString retrieveSequence( const QList< QByteArray >& mails,
                              const QList<int> &exceptions = QList<int>() ) const;
    QString deleteSequence( int numToDelete ) const;
    QString quitSequence() const;
    QString listSequence( const QList<QByteArray> &mails ) const;
    QString uidSequence( const QStringList &uids ) const;

    FakeServer *mFakeServer;
    OrgKdeAkonadiPOP3SettingsInterface *mSettingsInterface;
    Akonadi::Collection mMaildirCollection;
    QString mPop3Identifier;
    QString mMaildirPath;
};

#endif
