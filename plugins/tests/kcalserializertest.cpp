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

#include <akonadi/item.h>
#include <kcal/event.h>
#include <qtest_kde.h>
#include <QtCore/QObject>
#include <boost/shared_ptr.hpp>

using namespace Akonadi;
using namespace KCal;

class KCalSerializerTest : public QObject
{
  Q_OBJECT
  private slots:
    void testEventSerialize_data()
    {
      QTest::addColumn<QString>( "mimeType" );
      QTest::newRow( "specific" ) << "application/x-vnd.akonadi.calendar.event";
      QTest::newRow( "generic" ) << "text/calendar";
    }

    void testEventSerialize()
    {
      QFETCH( QString, mimeType );

      QByteArray serialized =
        "BEGIN:VCALENDAR\n"
        "PRODID:-//K Desktop Environment//NONSGML libkcal 3.5//EN\n"
        "VERSION:2.0\n"
        "BEGIN:VEVENT\n"
        "DTSTAMP:20070109T100625Z\n"
        "ORGANIZER;CN=\"Volker Krause\":MAILTO:vkrause@kde.org\n"
        "CREATED:20070109T100553Z\n"
        "UID:libkcal-1135684253.945\n"
        "SEQUENCE:1\n"
        "LAST-MODIFIED:20070109T100625Z\n"
        "SUMMARY:Test event\n"
        "LOCATION:here\n"
        "CLASS:PUBLIC\n"
        "PRIORITY:5\n"
        "CATEGORIES:KDE\n"
        "DTSTART:20070109T183000Z\n"
        "DTEND:20070109T225900Z\n"
        "TRANSP:OPAQUE\n"
        "BEGIN:VALARM\n"
        "DESCRIPTION:\n"
        "ACTION:DISPLAY\n"
        "TRIGGER;VALUE=DURATION:-PT45M\n"
        "END:VALARM\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";

      // deserializing
      Item item;
      item.setMimeType( mimeType );
      item.setPayloadFromData( serialized );

      QVERIFY( item.hasPayload<Event::Ptr>() );
      const Event::Ptr event = item.payload<Event::Ptr>();
      QVERIFY( event != 0 );

      QCOMPARE( event->summary(), QString( "Test event" ) );
      QCOMPARE( event->location(), QString( "here" ) );

      // serializing
      const QByteArray data = item.payloadData();
      QVERIFY( !data.isEmpty() );
    }
};

QTEST_KDEMAIN( KCalSerializerTest, NoGUI )

#include "kcalserializertest.moc"
