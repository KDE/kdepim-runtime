/*
    Copyright (C) 2009 Volker Krause <vkrause@kde.org>

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
#include "../collectionannotationsattribute.cpp"

typedef QMap<QByteArray, QByteArray> Annotation;
Q_DECLARE_METATYPE( Annotation )

using namespace Akonadi;

class CollectionAnnotationAttributeTest : public QObject
{
  Q_OBJECT
  private slots:
    void testSerializeDeserialize_data()
    {
      QTest::addColumn<Annotation>( "annotation" );

      Annotation a;
      QTest::newRow( "empty" ) << a;

      a.insert( "/vendor/cmu/cyrus-imapd/lastpop", "" );
      QTest::newRow( "empty value, single key" ) << a;

      a.insert( "/vendor/cmu/cyrus-imapd/condstore", "false" );
      QTest::newRow( "empty value, two keys" ) << a;

      a.insert( "/vendor/cmu/cyrus-imapd/sharedseen", "false" );
      QTest::newRow( "empty value, three keys" ) << a;
    }

    void testSerializeDeserialize()
    {
      QFETCH( Annotation, annotation );
      CollectionAnnotationsAttribute *attr1 = new CollectionAnnotationsAttribute();
      attr1->setAnnotations( annotation );
      QCOMPARE( attr1->annotations(), annotation );

      CollectionAnnotationsAttribute *attr2 = new CollectionAnnotationsAttribute();
      attr2->deserialize( attr1->serialized() );
      QCOMPARE( attr2->annotations(), annotation );

      delete attr1;
      delete attr2;
    }

};

QTEST_KDEMAIN( CollectionAnnotationAttributeTest, NoGUI )

#include "collectionannotationattributetest.moc"

