/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include <qtest_kde.h>

#include "selectsqarqlbuilder.h"

Q_DECLARE_METATYPE( SelectSparqlBuilder )

class SparqlBuilderTest : public QObject
{
  Q_OBJECT
  private slots:
    void testSelectBuilder_data()
    {
      QTest::addColumn<SelectSparqlBuilder>( "queryBuilder" );
      QTest::addColumn<QString>( "query" );

      SparqlBuilder::BasicGraphPattern graph;
      graph.addTriple( SparqlBuilder::TriplePattern( "$foo", QUrl("is"), "bar" ) );
      SelectSparqlBuilder qb;
      qb.addQueryVariable( "$foo" );
      qb.setGraphPattern( graph );
      QTest::newRow( "single basic graph pattern" ) << qb << QString( "SELECT $foo WHERE { $foo <is> \"bar\"^^<http://www.w3.org/2001/XMLSchema#string> }" );

      qb.addQueryVariable( "$foo2" );
      graph.addTriple( SparqlBuilder::TriplePattern( "$foo2", QUrl("has"), QUrl("blub") ) );
      qb.setGraphPattern( graph );
      QTest::newRow( "2 basic graph pattern" ) << qb << QString( "SELECT $foo, $foo2 WHERE { $foo <is> \"bar\"^^<http://www.w3.org/2001/XMLSchema#string> . $foo2 <has> <blub> }" );

      graph = SparqlBuilder::BasicGraphPattern();
      graph.addTriple( "$a", QUrl( "is" ), 10 );
      qb = SelectSparqlBuilder();
      qb.addQueryVariable( "$a" );
      qb.setDistinct( true );
      qb.setGraphPattern( graph );
      QTest::newRow( "distinct, int value" ) << qb << QString( "SELECT DISTINCT $a WHERE { $a <is> \"10\"^^<http://www.w3.org/2001/XMLSchema#int> }" );

      graph.addTriple( QUrl( "foo" ), QUrl("is"), SparqlBuilder::QueryVariable("$a") );
      qb.setGraphPattern( graph );
      QTest::newRow( "variable object" ) << qb << QString( "SELECT DISTINCT $a WHERE { $a <is> \"10\"^^<http://www.w3.org/2001/XMLSchema#int> . <foo> <is> $a }" );
    }

    void testSelectBuilder()
    {
      QFETCH( SelectSparqlBuilder, queryBuilder );
      QFETCH( QString, query );

      QCOMPARE( queryBuilder.query(), query );
    }
};

QTEST_KDEMAIN( SparqlBuilderTest, NoGUI )

#include "sparqlbuildertest.moc"
