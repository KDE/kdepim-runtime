/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "imapparser.h"
#include "imapparsertest.h"
#include <qtest_kde.h>

#include <QtCore/QDebug>
#include <QtCore/QVariant>

Q_DECLARE_METATYPE( QList<QByteArray> )
Q_DECLARE_METATYPE( QList<int> )

using namespace Akonadi;

QTEST_KDEMAIN( ImapParserTest, NoGUI )

void ImapParserTest::testStripLeadingSpaces( )
{
  QByteArray input = "  a  ";
  int result;

  // simple leading spaces at the beginning
  result = ImapParser::stripLeadingSpaces( input, 0 );
  QCOMPARE( result, 2 );

  // simple leading spaces in the middle
  result = ImapParser::stripLeadingSpaces( input, 1 );
  QCOMPARE( result, 2 );

  // no leading spaces
  result = ImapParser::stripLeadingSpaces( input, 2 );
  QCOMPARE( result, 2 );

  // trailing spaces
  result = ImapParser::stripLeadingSpaces( input, 3 );
  QCOMPARE( result, 5 );

  // out of bounds access
  result = ImapParser::stripLeadingSpaces( input, input.length() );
  QCOMPARE( result, input.length() );
}

void ImapParserTest::testParseQuotedString( )
{
  QByteArray input = "\"quoted \\\"NIL\\\"   string inside\"";
  QByteArray result;
  int consumed;

  // the whole thing
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QCOMPARE( result, QByteArray( "quoted \"NIL\"   string inside" ) );
  QCOMPARE( consumed, 32 );

  // unqoted string
  consumed = ImapParser::parseQuotedString( input, result, 1 );
  QCOMPARE( result, QByteArray( "quoted" ) );
  QCOMPARE( consumed, 7 );

  // whitespaces in qouted string
  consumed = ImapParser::parseQuotedString( input, result, 14 );
  QCOMPARE( result, QByteArray( "   string inside" ) );
  QCOMPARE( consumed, 32 );

  // whitespaces before unquoted string
  consumed = ImapParser::parseQuotedString( input, result, 15 );
  QCOMPARE( result, QByteArray( "string" ) );
  QCOMPARE( consumed, 24 );


  // NIL and emptyness tests
  input = "NIL \"NIL\" \"\"";

  // unqoted NIL
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QVERIFY( result.isNull() );
  QCOMPARE( consumed, 3 );

  // quoted NIL
  consumed = ImapParser::parseQuotedString( input, result, 3 );
  QCOMPARE( result, QByteArray( "NIL" ) );
  QCOMPARE( consumed, 9 );

  // quoted empty string
  consumed = ImapParser::parseQuotedString( input, result, 9 );
  QCOMPARE( result, QByteArray( "" ) );
  QCOMPARE( consumed, 12 );

  // unquoted string at input end
  input = "some string";
  consumed = ImapParser::parseQuotedString( input, result, 4 );
  QCOMPARE( result, QByteArray( "string" ) );
  QCOMPARE( consumed, 11 );

  // out of bounds access
  consumed = ImapParser::parseQuotedString( input, result, input.length() );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, input.length() );

  // de-quoting
  input = "\"\\\"some \\\\ quoted stuff\\\"\"";
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QCOMPARE( result, QByteArray( "\"some \\ quoted stuff\"" ) );
  QCOMPARE( consumed, input.length() );

  // linebreak as separator
  input = "LOGOUT\nFOO";
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QCOMPARE( result, QByteArray("LOGOUT") );
  QCOMPARE( consumed, 6 );
}

void ImapParserTest::testParseString( )
{
  QByteArray input = "\"quoted\" unquoted {7}\nliteral {0}\n empty literal";
  QByteArray result;
  int consumed;

  // quoted strings
  consumed = ImapParser::parseString( input, result, 0 );
  QCOMPARE( result, QByteArray( "quoted" ) );
  QCOMPARE( consumed, 8 );

  // unquoted string
  consumed = ImapParser::parseString( input, result, 8 );
  QCOMPARE( result, QByteArray( "unquoted" ) );
  QCOMPARE( consumed, 17 );

  // literal string
  consumed = ImapParser::parseString( input, result, 17 );
  QCOMPARE( result, QByteArray( "literal" ) );
  QCOMPARE( consumed, 29 );

  // empty literal string
  consumed = ImapParser::parseString( input, result, 29 );
  QCOMPARE( result, QByteArray( "" ) );
  QCOMPARE( consumed, 34 );

  // out of bounds access
  consumed = ImapParser::parseString( input, result, input.length() );
  QCOMPARE( result, QByteArray() );
  QCOMPARE( consumed, input.length() );
}

void ImapParserTest::testParseParenthesizedList( )
{
  QByteArray input;
  QList<QByteArray> result;
  int consumed;

  // empty lists
  input = "() ( )";
  consumed = ImapParser::parseParenthesizedList( input, result, 0 );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, 2 );

  consumed = ImapParser::parseParenthesizedList( input, result, 2 );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, 6 );

  // complex list with all kind of entries
  input = "(entry1 \"entry2()\" (sub list) \")))\" {6}\nentry3) end";
  consumed = ImapParser::parseParenthesizedList( input, result, 0 );
  QList<QByteArray> reference;
  reference << "entry1";
  reference << "entry2()";
  reference << "(sub list)";
  reference << ")))";
  reference << "entry3";
  QCOMPARE( result, reference );
  QCOMPARE( consumed, 47 );

  // no list at all
  input = "some list-less text";
  consumed = ImapParser::parseParenthesizedList( input, result, 0 );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, 0 );

  // out of bounds access
  consumed = ImapParser::parseParenthesizedList( input, result, input.length() );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, input.length() );

  // newline literal (based on itemappendtest bug)
  input = "(foo {6}\n\n\nbar\n bla)";
  consumed = ImapParser::parseParenthesizedList( input, result );
  reference.clear();
  reference << "foo";
  reference << "\n\nbar\n";
  reference << "bla";
  qDebug() << result;
  qDebug() << reference;
  QCOMPARE( result, reference );
  QCOMPARE( consumed, input.length() );
}

void ImapParserTest::testParseNumber()
{
  QByteArray input = " 1a23.4";
  int result, pos;
  bool ok;

  // empty string
  pos = ImapParser::parseNumber( QByteArray(), result, &ok );
  QCOMPARE( ok, false );
  QCOMPARE( pos, 0 );

  // leading spaces at the beginning
  pos = ImapParser::parseNumber( input, result, &ok, 0 );
  QCOMPARE( ok, true );
  QCOMPARE( pos, 2 );
  QCOMPARE( result, 1 );

  // multiple digits
  pos = ImapParser::parseNumber( input, result, &ok, 3 );
  QCOMPARE( ok, true );
  QCOMPARE( pos, 5 );
  QCOMPARE( result, 23 );

  // number at input end
  pos = ImapParser::parseNumber( input, result, &ok, 6 );
  QCOMPARE( ok, true );
  QCOMPARE( pos, 7 );
  QCOMPARE( result, 4 );

  // out of bounds access
  pos = ImapParser::parseNumber( input, result, &ok, input.length() );
  QCOMPARE( ok, false );
  QCOMPARE( pos, input.length() );
}

void ImapParserTest::testQuote_data()
{
  QTest::addColumn<QByteArray>( "unquoted" );
  QTest::addColumn<QByteArray>( "quoted" );

  QTest::newRow( "empty" ) << QByteArray("") << QByteArray("\"\"");
  QTest::newRow( "simple" ) << QByteArray("bla") << QByteArray("\"bla\"");
  QTest::newRow( "double-quotes" ) << QByteArray("\"test\"test\"") << QByteArray("\"\\\"test\\\"test\\\"\"");
  QTest::newRow( "backslash" ) << QByteArray("\\") << QByteArray("\"\\\\\"");
  QByteArray binaryNonEncoded;
  binaryNonEncoded += '\000';
  QByteArray binaryEncoded("\"" );
  binaryEncoded += '\000';
  binaryEncoded += '"';
  QTest::newRow( "binary" ) << binaryNonEncoded << binaryEncoded;
}

void ImapParserTest::testQuote()
{
  QFETCH( QByteArray, unquoted );
  QFETCH( QByteArray, quoted );
  QCOMPARE( ImapParser::quote( unquoted ), quoted );
}

void ImapParserTest::testMessageParser_data()
{
  QTest::addColumn<QList<QByteArray> >( "input" );
  QTest::addColumn<QByteArray>( "tag" );
  QTest::addColumn<QByteArray>( "data" );
  QTest::addColumn<bool>( "complete" );
  QTest::addColumn<QList<int> >( "continuations" );

  QList<QByteArray> input;
  QList<int> continuations;
  QTest::newRow( "empty" ) << input << QByteArray() << QByteArray() << false << continuations;

  input << "*";
  QTest::newRow( "tag-only" ) << input << QByteArray("*") << QByteArray() << true << continuations;

  input.clear();
  input << "20 UID FETCH (foo)";
  QTest::newRow( "simple" ) << input << QByteArray("20")
      << QByteArray( "UID FETCH (foo)" ) << true << continuations;

  input.clear();
  input << "1 (bla (" << ") blub)";
  QTest::newRow( "parenthesis" ) << input << QByteArray("1")
      << QByteArray( "(bla () blub)" ) << true << continuations;

  input.clear();
  input << "1 {3}" << "bla";
  continuations << 0;
  QTest::newRow( "literal" ) << input << QByteArray("1") << QByteArray("{3}bla")
      << true << continuations;

  input.clear();
  input << "1 FETCH (UID 5 DATA {3}"
        << "bla"
        << " RID 5)";
  QTest::newRow( "parenthesisEnclosedLiteral" ) << input << QByteArray("1")
      << QByteArray( "FETCH (UID 5 DATA {3}bla RID 5)" ) << true << continuations;

  input.clear();
  input << "1 {3}" << "bla {4}" << "blub";
  continuations.clear();
  continuations << 0 << 1;
  QTest::newRow( "2literal" ) << input << QByteArray("1")
      << QByteArray("{3}bla {4}blub") << true << continuations;

  input.clear();
  input << "1 {4}" << "A{9}";
  continuations.clear();
  continuations << 0;
  QTest::newRow( "literal in literal" ) << input << QByteArray("1")
      << QByteArray("{4}A{9}") << true << continuations;

  input.clear();
  input << "* FETCH (UID 1 DATA {3}" << "bla" << " ENVELOPE {4}" << "blub" << " RID 5)";
  continuations.clear();
  continuations << 0 << 2;
  QTest::newRow( "enclosed2literal" ) << input << QByteArray("*")
      << QByteArray( "FETCH (UID 1 DATA {3}bla ENVELOPE {4}blub RID 5)" )
      << true << continuations;

  input.clear();
  input << "1 DATA {0}";
  continuations.clear();
  QTest::newRow( "empty literal" ) << input << QByteArray( "1" )
      << QByteArray( "DATA {0}" ) << true << continuations;
}

void ImapParserTest::testMessageParser()
{
  QFETCH( QList<QByteArray>, input );
  QFETCH( QByteArray, tag );
  QFETCH( QByteArray, data );
  QFETCH( bool, complete );
  QFETCH( QList<int>, continuations );
  QList<int> cont = continuations;

  ImapParser *parser = new ImapParser();
  QVERIFY( parser->tag().isEmpty() );
  QVERIFY( parser->data().isEmpty() );

  for ( int i = 0; i < input.count(); ++i ) {
    bool res = parser->parseNextLine( input.at( i ) );
    if ( i != input.count() - 1 )
      QVERIFY( !res );
    else
      QCOMPARE( res, complete );
    if ( parser->continuationStarted() ) {
      QVERIFY( cont.contains( i ) );
      cont.removeAll( i );
    }
  }

  QCOMPARE( parser->tag(), tag );
  QCOMPARE( parser->data(), data );
  QVERIFY( cont.isEmpty() );

  // try again, this time with a not fresh parser
  parser->reset();
  QVERIFY( parser->tag().isEmpty() );
  QVERIFY( parser->data().isEmpty() );
  cont = continuations;

  for ( int i = 0; i < input.count(); ++i ) {
    bool res = parser->parseNextLine( input.at( i ) );
    if ( i != input.count() - 1 )
      QVERIFY( !res );
    else
      QCOMPARE( res, complete );
    if ( parser->continuationStarted() ) {
      QVERIFY( cont.contains( i ) );
      cont.removeAll( i );
    }
  }

  QCOMPARE( parser->tag(), tag );
  QCOMPARE( parser->data(), data );
  QVERIFY( cont.isEmpty() );

  delete parser;
}

#include "imapparsertest.moc"
