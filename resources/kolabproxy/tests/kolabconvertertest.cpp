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

#include <QObject>
#include <qtest_kde.h>
#include <QDir>
#include <akonadi/item.h>
#include <kmime/kmime_message.h>
#include <kolabhandler.h>
#include <kabc/vcardconverter.h>
#include <algorithm>

using namespace Akonadi;
using namespace KMime;

static Akonadi::Item readMimeFile( const QString &fileName )
{
  qDebug() << fileName;
  QFile file( fileName );
  file.open( QFile::ReadOnly );
  const QByteArray data = file.readAll();
  Q_ASSERT( !data.isEmpty() );

  KMime::Message *msg = new KMime::Message;
  msg->setContent( data );
  msg->parse();

  Akonadi::Item item( "message/rfc822" );
  item.setPayload( KMime::Message::Ptr( msg ) );
  return item;
}

#define KCOMPARE(actual, expected) \
do {\
    if ( !(actual == expected) ) { \
      qDebug() << __FILE__ << ':' << __LINE__ << "Actual: " #actual ": " << actual << "\nExpceted: " #expected ": " << expected; \
      return false; \
   } \
} while (0)

static bool compareMimeMessage( const KMime::Message::Ptr &msg, const KMime::Message::Ptr &expectedMsg )
{
  // headers
  KCOMPARE( msg->subject()->asUnicodeString(), expectedMsg->subject()->asUnicodeString() );
  KCOMPARE( msg->from()->asUnicodeString(), expectedMsg->from()->asUnicodeString() );
  KCOMPARE( msg->contentType()->mimeType(), expectedMsg->contentType()->mimeType() );
  KCOMPARE( msg->headerByType( "X-Kolab-Type" )->as7BitString(), expectedMsg->headerByType( "X-Kolab-Type" )->as7BitString() );
  // date contains conversion time...
//   KCOMPARE( msg->date()->asUnicodeString(), expectedMsg->date()->asUnicodeString() );

  // body parts
  KCOMPARE( msg->contents().size(), expectedMsg->contents().size() );
  for ( int i = 0; i < msg->contents().size(); ++i ) {
    KMime::Content *part = msg->contents().at( i );
    KMime::Content *expectedPart = msg->contents().at( i );

    // part headers
    KCOMPARE( part->contentType()->mimeType(), expectedPart->contentType()->mimeType() );
    KCOMPARE( part->contentDisposition()->filename(), expectedPart->contentDisposition()->filename() );

    // part content
    KCOMPARE( part->decodedContent(), expectedPart->decodedContent() );
  }
  return true;
}

static bool LexicographicalPhoneNumberCompare( const KABC::PhoneNumber &p1, const KABC::PhoneNumber &p2 )
{
  if ( p1.type() == p2.type() )
    return p1.number() < p2.number();
  return p1.type() < p2.type();
}

static bool alignPhoneNumbers( KABC::Addressee &addressee, const KABC::Addressee &refAddressee )
{
  KABC::PhoneNumber::List phoneNumbers = addressee.phoneNumbers();
  KABC::PhoneNumber::List refPhoneNumbers = refAddressee.phoneNumbers();
  if ( phoneNumbers.size() != refPhoneNumbers.size() )
    return false;
  std::sort( phoneNumbers.begin(), phoneNumbers.end(), LexicographicalPhoneNumberCompare );
  std::sort( refPhoneNumbers.begin(), refPhoneNumbers.end(), LexicographicalPhoneNumberCompare );

  for ( int i = 0; i < phoneNumbers.size(); ++i ) {
    KABC::PhoneNumber phoneNumber = phoneNumbers.at( i );
    const KABC::PhoneNumber refPhoneNumber = refPhoneNumbers.at( i );
    KCOMPARE( phoneNumber.type(), refPhoneNumber.type() );
    KCOMPARE( phoneNumber.number(), refPhoneNumber.number() );
    addressee.removePhoneNumber( phoneNumber );
    phoneNumber.setId( refPhoneNumber.id() );
    addressee.insertPhoneNumber( phoneNumber );
  }

  return true;
}

class KolabConverterTest : public QObject
{
  Q_OBJECT
  private slots:
    void testContacts_data()
    {
      QTest::addColumn<QString>( "vcardFileName" );
      QTest::addColumn<QString>( "mimeFileName" );

      const QDir dir( KDESRCDIR "/contacts" );
      const QStringList entries = dir.entryList( QStringList("*.vcf"), QDir::Files | QDir::Readable | QDir::NoSymLinks );
      foreach( const QString &entry, entries ) {
        QTest::newRow( QString::fromLatin1( "contact-%1" ).arg( entry ).toLatin1() ) << (dir.path() + '/' + entry) << QString::fromLatin1( "%1/%2.mime" ).arg( dir.path() ).arg( entry );
      }
    }

    void testContacts()
    {
      QFETCH( QString, vcardFileName );
      QFETCH( QString, mimeFileName );

      KolabHandler *handler = KolabHandler::createHandler( "contact" );
      QVERIFY( handler );

      // mime -> vcard conversion
      const Item kolabItem = readMimeFile( mimeFileName );
      QVERIFY( kolabItem.hasPayload() );

      Item::List vcardItems = handler->translateItems( Akonadi::Item::List() << kolabItem );
      QCOMPARE( vcardItems.size(), 1 );
      QVERIFY( vcardItems.first().hasPayload<KABC::Addressee>() );
      KABC::Addressee convertedAddressee = vcardItems.first().payload<KABC::Addressee>();

      QFile vcardFile( vcardFileName );
      QVERIFY( vcardFile.open( QFile::ReadOnly ) );
      KABC::VCardConverter m_converter;
      const KABC::Addressee realAddressee = m_converter.parseVCard( vcardFile.readAll() );

      // fix up the converted addressee for comparisson
      convertedAddressee.setName( realAddressee.name() ); // name() apparently is something strange
      QVERIFY( !convertedAddressee.custom( "KOLAB", "CreationDate" ).isEmpty() );
      convertedAddressee.removeCustom( "KOLAB", "CreationDate" ); // that's conversion time !?
      QVERIFY( alignPhoneNumbers( convertedAddressee, realAddressee ) ); // phone number ids are random

//       qDebug() << convertedAddressee.toString();
//       qDebug() << realAddressee.toString();
      QCOMPARE( realAddressee, convertedAddressee );


      // and now the other way around
      Item convertedKolabItem;
      Item vcardItem( "text/directory" );
      vcardItem.setPayload( realAddressee );
      handler->toKolabFormat( vcardItem, convertedKolabItem );
      QVERIFY( convertedKolabItem.hasPayload<KMime::Message::Ptr>() );

      const KMime::Message::Ptr convertedMime = convertedKolabItem.payload<KMime::Message::Ptr>();
      const KMime::Message::Ptr realMime = kolabItem.payload<KMime::Message::Ptr>();

//       qDebug() << convertedMime->encodedContent();
//       qDebug() << realMime->encodedContent();
      QVERIFY( compareMimeMessage( convertedMime, realMime ) );

      delete handler;
    }
};

QTEST_KDEMAIN( KolabConverterTest, NoGUI )

#include "kolabconvertertest.moc"
