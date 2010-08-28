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
#include <kcalcore/incidence.h>
#include <kcalcore/icalformat.h>
#include <kcalcore/todo.h>

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
  if ( msg->from()->isEmpty() || expectedMsg->from()->isEmpty() )
    KCOMPARE( msg->from()->asUnicodeString(), expectedMsg->from()->asUnicodeString() );
  else
    KCOMPARE( msg->from()->mailboxes().first().address(), expectedMsg->from()->mailboxes().first().address() ); // matching address is enough, we don't need a display name
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

template <template <typename> class Op, typename T>
static bool LexicographicalCompare( const T &_x, const T &_y )
{
  T x( _x );
  x.setId( QString() );
  T y( _y );
  y.setId( QString() );
  Op<QString> op;
  return op( x.toString(), y.toString() );
}

static bool normalizePhoneNumbers( KABC::Addressee &addressee, const KABC::Addressee &refAddressee )
{
  KABC::PhoneNumber::List phoneNumbers = addressee.phoneNumbers();
  KABC::PhoneNumber::List refPhoneNumbers = refAddressee.phoneNumbers();
  if ( phoneNumbers.size() != refPhoneNumbers.size() )
    return false;
  std::sort( phoneNumbers.begin(), phoneNumbers.end(), LexicographicalCompare<std::less, KABC::PhoneNumber> );
  std::sort( refPhoneNumbers.begin(), refPhoneNumbers.end(), LexicographicalCompare<std::less, KABC::PhoneNumber> );

  for ( int i = 0; i < phoneNumbers.size(); ++i ) {
    KABC::PhoneNumber phoneNumber = phoneNumbers.at( i );
    const KABC::PhoneNumber refPhoneNumber = refPhoneNumbers.at( i );
    KCOMPARE( LexicographicalCompare<std::equal_to>( phoneNumber, refPhoneNumber ), true );
    addressee.removePhoneNumber( phoneNumber );
    phoneNumber.setId( refPhoneNumber.id() );
    addressee.insertPhoneNumber( phoneNumber );
  }

  return true;
}

static bool normalizeAddresses( KABC::Addressee &addressee, const KABC::Addressee &refAddressee )
{
  KABC::Address::List addresses = addressee.addresses();
  KABC::Address::List refAddresses = refAddressee.addresses();
  if ( addresses.size() != refAddresses.size() )
    return false;
  std::sort( addresses.begin(), addresses.end(), LexicographicalCompare<std::less, KABC::Address> );
  std::sort( refAddresses.begin(), refAddresses.end(), LexicographicalCompare<std::less, KABC::Address> );

  for ( int i = 0; i < addresses.size(); ++i ) {
    KABC::Address address = addresses.at( i );
    const KABC::Address refAddress = refAddresses.at( i );
    KCOMPARE( LexicographicalCompare<std::equal_to>( address, refAddress ), true );
    addressee.removeAddress( address );
    address.setId( refAddress.id() );
    addressee.insertAddress( address );
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
      KABC::VCardConverter converter;
      const KABC::Addressee realAddressee = converter.parseVCard( vcardFile.readAll() );

      // fix up the converted addressee for comparisson
      convertedAddressee.setName( realAddressee.name() ); // name() apparently is something strange
      QVERIFY( !convertedAddressee.custom( "KOLAB", "CreationDate" ).isEmpty() );
      convertedAddressee.removeCustom( "KOLAB", "CreationDate" ); // that's conversion time !?
      QVERIFY( normalizePhoneNumbers( convertedAddressee, realAddressee ) ); // phone number ids are random
      QVERIFY( normalizeAddresses( convertedAddressee, realAddressee ) ); // same here

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

    void testIncidences_data()
    {
      QTest::addColumn<QString>( "type" );
      QTest::addColumn<QString>( "icalFileName" );
      QTest::addColumn<QString>( "mimeFileName" );

      const QStringList types = QStringList() << "event" << "task" << "journal" << "note";

      foreach ( const QString &type, types ) {
        const QDir dir( QString::fromLatin1( "%1/%2" ).arg( KDESRCDIR ).arg( type ) );
        const QStringList entries = dir.entryList( QStringList("*.ics"), QDir::Files | QDir::Readable | QDir::NoSymLinks );
        foreach( const QString &entry, entries ) {
          QTest::newRow( QString::fromLatin1( "%1-%2" ).arg( type ).arg( entry ).toLatin1() ) << type << (dir.path() + '/' + entry) << QString::fromLatin1( "%1/%2.mime" ).arg( dir.path() ).arg( entry );
        }
      }
    }

    void testIncidences()
    {
      QFETCH( QString, type );
      QFETCH( QString, icalFileName );
      QFETCH( QString, mimeFileName );

      KolabHandler *handler = KolabHandler::createHandler( type.toLatin1() );
      QVERIFY( handler );

      // mime -> vcard conversion
      const Item kolabItem = readMimeFile( mimeFileName );
      QVERIFY( kolabItem.hasPayload() );

      Item::List icalItems = handler->translateItems( Akonadi::Item::List() << kolabItem );
      QCOMPARE( icalItems.size(), 1 );
      QVERIFY( icalItems.first().hasPayload<KCalCore::Incidence::Ptr>() );
      KCalCore::Incidence::Ptr convertedIncidence = icalItems.first().payload<KCalCore::Incidence::Ptr>();

      QFile icalFile( icalFileName );
      QVERIFY( icalFile.open( QFile::ReadOnly ) );
      KCalCore::ICalFormat format;
      const KCalCore::Incidence::Ptr realIncidence( format.fromString( QString::fromUtf8( icalFile.readAll() ) ) );

      // fix up the converted incidence for comparisson
      if ( type == "task" ) {
        QVERIFY( icalItems.first().hasPayload<KCalCore::Todo::Ptr>() );
        KCalCore::Todo::Ptr todo = icalItems.first().payload<KCalCore::Todo::Ptr>();
        if ( !todo->hasDueDate() && !todo->hasStartDate() )
          convertedIncidence->setAllDay( realIncidence->allDay() ); // all day has no meaning if there are no start and due dates but may differ nevertheless
      }
      // recurrence objects are created on demand, but KCalCore::Incidence::operator==() doesn't take that into account
      // so make sure both incidences have one
      realIncidence->recurrence();
      convertedIncidence->recurrence();

      if ( *(realIncidence.data()) != *(convertedIncidence.data()) ) {
        qDebug() << "REAL: " << format.toString( realIncidence );
        qDebug() << "CONVERTED: " << format.toString( convertedIncidence );
      }
      QVERIFY( *(realIncidence.data()) ==  *(convertedIncidence.data()) );


      // and now the other way around
      Item convertedKolabItem;
      Item icalItem;
      switch ( realIncidence->type() ) {
        case KCalCore::IncidenceBase::TypeEvent: icalItem.setMimeType( KCalCore::Event::eventMimeType() ); return;
        case KCalCore::IncidenceBase::TypeTodo: icalItem.setMimeType( KCalCore::Todo::todoMimeType() ); return;
        case KCalCore::IncidenceBase::TypeJournal: icalItem.setMimeType( KCalCore::Journal::journalMimeType() ); return;
        default: QFAIL( "incidence type not supported" );
      }
      icalItem.setPayload( realIncidence );
      handler->toKolabFormat( icalItem, convertedKolabItem );
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
