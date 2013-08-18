/*
    Copyright (C) 2011-2013 Daniel Vrátil <dvratil@redhat.com>

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

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <LibKGAPI2/Calendar/Reminder>

using namespace KGAPI2;

DefaultReminderAttribute::DefaultReminderAttribute()
{
}

Akonadi::Attribute *DefaultReminderAttribute::clone() const
{
    DefaultReminderAttribute *attr = new DefaultReminderAttribute();
    attr->setReminders( m_reminders );

    return attr;
}

void DefaultReminderAttribute::setReminders( const RemindersList &reminders )
{
    m_reminders = reminders;
}

void DefaultReminderAttribute::deserialize( const QByteArray &data )
{
    QJson::Parser parser;
    QVariantList list;

    list = parser.parse( data ).toList();
    Q_FOREACH( const QVariant & l, list ) {
        QVariantMap reminder = l.toMap();

        KGAPI2::ReminderPtr rem( new KGAPI2::Reminder );

        if ( reminder[QLatin1String("type")].toString() == QLatin1String("display") ) {
            rem->setType( KCalCore::Alarm::Display );
        } else if ( reminder[QLatin1String("type")].toString() == QLatin1String("email") ) {
            rem->setType( KCalCore::Alarm::Email );
        }

        KCalCore::Duration offset( reminder[QLatin1String("time")].toInt(), KCalCore::Duration::Seconds );
        rem->setStartOffset( offset );

        m_reminders << rem;
    }
}

QByteArray DefaultReminderAttribute::serialized() const
{
    QVariantList list;

    Q_FOREACH( const ReminderPtr & rem, m_reminders ) {
        QVariantMap reminder;

        if ( rem->type() == KCalCore::Alarm::Display ) {
            reminder[QLatin1String("type")] = QLatin1String("display");
        } else if ( rem->type() == KCalCore::Alarm::Email ) {
            reminder[QLatin1String("type")] = QLatin1String("email");
        }

        reminder[QLatin1String("time")] = rem->startOffset().asSeconds();

        list << reminder;
    }

    QJson::Serializer serializer;
    return serializer.serialize( list );
}

KCalCore::Alarm::List DefaultReminderAttribute::alarms( KCalCore::Incidence *incidence ) const
{
    KCalCore::Alarm::List alarms;

    Q_FOREACH( const ReminderPtr & reminder, m_reminders ) {
        KCalCore::Alarm::Ptr alarm( new KCalCore::Alarm( incidence ) );

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

