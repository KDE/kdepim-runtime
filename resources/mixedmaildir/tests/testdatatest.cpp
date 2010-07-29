/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "testdatautil.h"

#include <KTempDir>

#include <qtest_kde.h>

class TestDataTest : public QObject
{
  Q_OBJECT
  public:
    TestDataTest() {}

  private Q_SLOTS:
    void testResources();
    void testInstall();
};

void TestDataTest::testResources()
{
  const QStringList testDataNames = TestDataUtil::testDataNames();
  QCOMPARE( testDataNames, QStringList() << QLatin1String( "dimap" )
                                         << QLatin1String( "maildir" )
                                         << QLatin1String( "maildir-tagged" )
                                         << QLatin1String( "mbox" )
                                         << QLatin1String( "mbox-tagged" )
                                         << QLatin1String( "mbox-unpurged" ) );

  Q_FOREACH( const QString testDataName, testDataNames ) {
    if ( testDataName.startsWith( QLatin1String( "mbox" ) ) ) {
      QVERIFY( TestDataUtil::folderType( testDataName ) == TestDataUtil::MBoxFolder );
    } else {
      QVERIFY( TestDataUtil::folderType( testDataName ) == TestDataUtil::MaildirFolder );
    }
  }

  // TODO check contents?
}

void TestDataTest::testInstall()
{
  KTempDir dir;
  QDir installDir( dir.name() );
  QDir curDir;

  const QString indexFilePattern = QLatin1String( ".%1.index" );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), dir.name(), QLatin1String( "mbox1" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "mbox1" ) ) );
  QVERIFY( installDir.exists( indexFilePattern.arg( QLatin1String( "mbox1" ) ) ) );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox-tagged" ), dir.name(), QLatin1String( "mbox2" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "mbox2" ) ) );
  QVERIFY( installDir.exists( indexFilePattern.arg( QLatin1String( "mbox2" ) ) ) );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), dir.name(), QLatin1String( "md1" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md1" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md1/new" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md1/cur" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md1/tmp" ) ) );
  QVERIFY( installDir.exists( indexFilePattern.arg( QLatin1String( "md1" ) ) ) );

  curDir = installDir;
  curDir.cd( QLatin1String( "md1" ) );
  curDir.cd( QLatin1String( "cur" ) );
  curDir.setFilter( QDir::Files );
  QCOMPARE( (int)curDir.count(), 4 );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir-tagged" ), dir.name(), QLatin1String( "md2" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md2" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md2/new" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md2/cur" ) ) );
  QVERIFY( installDir.exists( QLatin1String( "md2/tmp" ) ) );
  QVERIFY( installDir.exists( indexFilePattern.arg( QLatin1String( "md2" ) ) ) );

  curDir = installDir;
  curDir.cd( QLatin1String( "md2" ) );
  curDir.cd( QLatin1String( "cur" ) );
  curDir.setFilter( QDir::Files );
  QCOMPARE( (int)curDir.count(), 4 );
}

#include "testdatatest.moc"

QTEST_KDEMAIN( TestDataTest, NoGUI )

// kate: space-indent on; indent-width 2; replace-tabs on;
