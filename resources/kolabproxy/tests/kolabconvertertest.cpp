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
#include <akonadi/item.h>
#include <kmime/kmime_message.h>
#include <kolabhandler.h>
#include <algorithm>
#include <kcalcore/incidence.h>
#include <kcalcore/todo.h>

using namespace Akonadi;
using namespace KMime;
Q_DECLARE_METATYPE(KCalCore::Incidence::Ptr)

class KolabConverterTest : public QObject
{
  Q_OBJECT
  private slots:
    void testContacts_data()
    {
      QTest::addColumn<KABC::Addressee>( "addressee" );
      KABC::Addressee contact;
      contact.setName(QLatin1String("John Doe"));
      contact.setEmails(QStringList() << QLatin1String("doe@example.org"));

      QTest::newRow( "contact" ) << contact;
    }

    void testContacts()
    {
      QFETCH( KABC::Addressee, addressee );

      KolabHandler::Ptr handler = KolabHandler::createHandler( Kolab::ContactType, Collection() );
      QVERIFY( handler );

      Item vcardItem( QLatin1String("text/directory") );
      vcardItem.setPayload( addressee );

      //Convert to kolab item
      Item kolabItem;
      handler->toKolabFormat( vcardItem, kolabItem );
      QVERIFY( kolabItem.hasPayload<KMime::Message::Ptr>() );

      //and back
      Item::List vcardItems = handler->translateItems( Akonadi::Item::List() << kolabItem );
      QCOMPARE( vcardItems.size(), 1 );
      QVERIFY( vcardItems.first().hasPayload<KABC::Addressee>() );
    }

    void testIncidences_data()
    {
      QTest::addColumn<QString>( "type" );
      QTest::addColumn<KCalCore::Incidence::Ptr>( "incidence" );

      KCalCore::Incidence::Ptr event(new KCalCore::Event);
      event->setDtStart(KDateTime::currentUtcDateTime());
      QTest::newRow("event") << "event" << event;
      
      KCalCore::Incidence::Ptr todo(new KCalCore::Todo);
      todo->setDtStart(KDateTime::currentUtcDateTime());
      QTest::newRow("todo") << "task" << todo;
      
      KCalCore::Incidence::Ptr journal(new KCalCore::Journal);
      journal->setDtStart(KDateTime::currentUtcDateTime());
      QTest::newRow("journal") << "journal" << journal;
    }

    void testIncidences()
    {
      QFETCH( QString, type );
      QFETCH( KCalCore::Incidence::Ptr, incidence );

      KolabHandler::Ptr handler = KolabHandler::createHandler( Kolab::folderTypeFromString( type.toLatin1() ), Collection() );
      QVERIFY( handler );

      Item icalItem;
      switch ( incidence->type() ) {
        case KCalCore::IncidenceBase::TypeEvent: icalItem.setMimeType( KCalCore::Event::eventMimeType() ); return;
        case KCalCore::IncidenceBase::TypeTodo: icalItem.setMimeType( KCalCore::Todo::todoMimeType() ); return;
        case KCalCore::IncidenceBase::TypeJournal: icalItem.setMimeType( KCalCore::Journal::journalMimeType() ); return;
        default: QFAIL( "incidence type not supported" );
      }
      icalItem.setPayload( incidence );

      // Convert to kolab item
      Item kolabItem;
      handler->toKolabFormat( icalItem, kolabItem );
      QVERIFY( kolabItem.hasPayload<KMime::Message::Ptr>() );

      //and back
      Item::List icalItems = handler->translateItems( Akonadi::Item::List() << kolabItem );
      QCOMPARE( icalItems.size(), 1 );
      QVERIFY( icalItems.first().hasPayload<KCalCore::Incidence::Ptr>() );
      if ( type == QLatin1String("task") ) {
        QVERIFY( icalItems.first().hasPayload<KCalCore::Todo::Ptr>() );
      }
    }
};

QTEST_KDEMAIN( KolabConverterTest, NoGUI )

#include "kolabconvertertest.moc"
