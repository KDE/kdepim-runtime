/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "defaultreminderattribute.h"

#include <QVariant>

#include <libkgoogle/objects/calendar.h>

#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace KCalCore;
using namespace KGoogle::Objects;

DefaultReminderAttribute::DefaultReminderAttribute( const Reminder::List &reminders )
  : m_reminders( reminders )
{
}

Akonadi::Attribute *DefaultReminderAttribute::clone() const
{
  return new DefaultReminderAttribute( m_reminders );
}

void DefaultReminderAttribute::deserialize( const QByteArray &data )
{
  QJson::Parser parser;
  QVariantList list;

  list = parser.parse( data ).toList();
  Q_FOREACH ( const QVariant & l, list ) {
    QVariantMap reminder = l.toMap();

    Reminder::Ptr rem( new Reminder );
    if ( reminder["type"].toString() == "display" ) {
      rem->setType( Alarm::Display );
    } else if ( reminder["type"].toString() == "email" ) {
      rem->setType( Alarm::Email );
    }

    Duration offset( reminder["time"].toInt(), Duration::Seconds );
    rem->setStartOffset( offset );

    m_reminders << rem;
  }
}

QByteArray DefaultReminderAttribute::serialized() const
{
  QVariantList list;

  Q_FOREACH ( Reminder::Ptr rem, m_reminders ) {
    QVariantMap reminder;

    if ( rem->type() == Alarm::Display ) {
      reminder["type"] = "display";
    } else if ( rem->type() == Alarm::Email ) {
      reminder["type"] = "email";
    }

    reminder["time"] = rem->startOffset().asSeconds();

    list << reminder;
  }

  QJson::Serializer serializer;
  return serializer.serialize( list );
}

Alarm::List DefaultReminderAttribute::alarms( Incidence *incidence ) const
{
  Alarm::List alarms;

  Q_FOREACH ( const Reminder::Ptr & reminder, m_reminders ) {
    AlarmPtr alarm( new Alarm( incidence ) );

    alarm->setType( reminder->type() );
    alarm->setTime( incidence->dtStart() );
    alarm->setStartOffset( reminder->startOffset() );
    alarm->setEnabled( true );

    alarms << alarm;
  }

  return alarms;
}

QByteArray DefaultReminderAttribute::type() const
{
  return "defaultReminders";
}

