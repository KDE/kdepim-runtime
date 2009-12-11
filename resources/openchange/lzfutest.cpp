/*
  Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; see the file COPYING.LIB.  If not,
  write to the Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "lzfutest.h"

#include "lzfu.h"

#include <qtest_kde.h>

QTEST_KDEMAIN( LzfuTest, NoGUI )

void LzfuTest::testDecompress_data()
{
  QTest::addColumn<QByteArray>( "compressed" );
  QTest::addColumn<QByteArray>( "uncompressed" );

  // test from Exchange 2007
  QByteArray compressed = QByteArray::fromHex( "d60000000b0100004c5a467556c587aa03000a0072637067313235163200f80b606e0e103033334f01f702a403e3020063680ac073b065743020071302807d0a809d00002a09b009f00490617405b11a520de068098001d020352e4035302e39392e01d031923402805c760890776b0b807464340c606300500b030bb520284a757305407303706520d7129014d003702005a06d07800230390ac0792e0aa20a840a80416eac6f74132005c06c0b806517dbc24f05c074776f2e0ae31a13fa6119132003f018d0086005401b405b026000706b191a11e1001da0" );
  QByteArray uncompressed( "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}}\r\n{\\*\\generator Riched20 5.50.99.2014;}\\viewkind4\\uc1\\pard\\f0\\fs20 Just some random commentary.\\par?\n\\par?\nAnother line.\\par?\n\\par?\nOr two. \\par?\nOr a line without a blank line.\\par?\n}}" );

  QTest::newRow("case1") << compressed << uncompressed; 

  compressed.clear();
  uncompressed.clear();
  QTest::newRow("empty") << compressed << uncompressed;
  compressed  = QByteArray::fromHex( "d60000000b0100004c5a467556c587aa" );
  QTest::newRow("justheader") << compressed << uncompressed;
  compressed  = QByteArray::fromHex( "d60000000b0100004c5a467556c587" );
  QTest::newRow("partheader") << compressed << uncompressed;    
}

void LzfuTest::testDecompress()
{
  QFETCH( QByteArray, compressed );
  QFETCH( QByteArray, uncompressed );

  QByteArray result = lzfuDecompress( compressed );
  qDebug() << "uncompressed: " << result;
  QCOMPARE( result.mid(0,19), uncompressed.mid(0,19) );
  QCOMPARE( result.mid(20,39), uncompressed.mid(20,39) );
  QCOMPARE( result.mid(40,49), uncompressed.mid(40,49) );
  QCOMPARE( result.mid(50,59), uncompressed.mid(50,59) );
  QCOMPARE( result.mid(60,69), uncompressed.mid(60,69) );

}

#include "lzfutest.moc"
