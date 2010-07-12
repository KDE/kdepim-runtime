/*
    Copyright (C) 2010 Tobias Koenig <tokoe@kde.org>

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

#include <QtTest>
#include <qtest_kde.h>
#include "../imapaclattribute.cpp"

using namespace Akonadi;

typedef QMap<QByteArray, KIMAP::Acl::Rights> ImapAcl;

Q_DECLARE_METATYPE( ImapAcl );

class ImapAclAttributeTest : public QObject
{
  Q_OBJECT

  private Q_SLOTS:

    void testSerializeDeserialize_data()
    {
      QTest::addColumn<ImapAcl>( "rights" );
      QTest::addColumn<QByteArray>( "serialized" );
      QTest::addColumn<QByteArray>( "oldSerialized" );

      ImapAcl acl;
      QTest::newRow( "empty" ) << acl << QByteArray( " %% " ) << QByteArray( "testme@host l %% " );

      acl.insert( "user@host", KIMAP::Acl::None );
      QTest::newRow( "none" ) << acl << QByteArray( "user@host  %% " ) << QByteArray( "testme@host l %% user@host " );

      acl.insert( "user@host", KIMAP::Acl::Lookup );
      QTest::newRow( "lookup" ) << acl << QByteArray( "user@host l %% " ) << QByteArray( "testme@host l %% user@host l" );

      acl.insert( "user@host", KIMAP::Acl::Lookup | KIMAP::Acl::Read );
      QTest::newRow( "lookup/read" ) << acl << QByteArray( "user@host lr %% " ) << QByteArray( "testme@host l %% user@host lr" );

      acl.insert( "otheruser@host", KIMAP::Acl::Lookup | KIMAP::Acl::Read );
      QTest::newRow( "lookup/read" ) << acl << QByteArray( "otheruser@host lr % user@host lr %% " )
                                     << QByteArray( "testme@host l %% otheruser@host lr % user@host lr" );
    }

    void testSerializeDeserialize()
    {
      QFETCH( ImapAcl, rights );
      QFETCH( QByteArray, serialized );
      QFETCH( QByteArray, oldSerialized );

      ImapAclAttribute *attr = new ImapAclAttribute();
      attr->setRights( rights );
      QCOMPARE( attr->serialized(), serialized );

      ImapAcl acl;
      acl.insert( "testme@host", KIMAP::Acl::Lookup );
      attr->setRights( acl );

      QCOMPARE( attr->serialized(), oldSerialized );

      delete attr;
    }

    void testOldRights()
    {
      ImapAcl acls;
      acls.insert( "first_user@host", KIMAP::Acl::Lookup | KIMAP::Acl::Read );
      acls.insert( "second_user@host", KIMAP::Acl::Lookup | KIMAP::Acl::Read );
      acls.insert( "third_user@host", KIMAP::Acl::Lookup | KIMAP::Acl::Read );

      ImapAclAttribute *attr = new ImapAclAttribute();
      attr->setRights( acls );

      ImapAcl oldAcls = acls;

      acls.remove( "first_user@host" );
      acls.remove( "third_user@host" );

      attr->setRights( acls );

      QCOMPARE( attr->oldRights(), oldAcls );

      attr->setRights( acls );

      QCOMPARE( attr->oldRights(), acls );
    }
};

QTEST_KDEMAIN( ImapAclAttributeTest, NoGUI )

#include "imapaclattributetest.moc"
