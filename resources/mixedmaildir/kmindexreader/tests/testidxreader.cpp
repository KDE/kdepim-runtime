/*
 *   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "testidxreader.h"

#include "TestIdxReader_data.h"
#include "../kmindexreader.h"

#include <QTemporaryFile>

#include <qtest_kde.h>
#include <QtTest/QTest>
#include <QDebug>

QTEST_MAIN( TestIdxReader )


TestIdxReader::TestIdxReader()
{  
}

void TestIdxReader::testError()
{
  KMIndexReader reader( "IDoNotExist" );
  
  QVERIFY( reader.error() == true );
}

void TestIdxReader::testReadHeader()
{
  QTemporaryFile tmp;
  if( !tmp.open() ) {
    qDebug() << "Could not open temp file.";
    return;
  }
  tmp.write( QByteArray::fromBase64(mailDirOneEmail) );
  tmp.close();
  KMIndexReader reader( tmp.fileName() );
  
  QVERIFY( reader.error() == false );
  
  int version = 0;
  bool success = reader.readHeader( &version );
  
  QVERIFY( success == true );
  QCOMPARE( version, 1506 );
  
  QVERIFY( reader.error() == false );
}
