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
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GOOGLE_CALENDAR_DEFAULTREMINDERATTRIBUTE_H
#define GOOGLE_CALENDAR_DEFAULTREMINDERATTRIBUTE_H

#include <AkonadiCore/Attribute>

#include <KGAPI/Types>

#include <KCalCore/Alarm>
#include <KCalCore/Incidence>

class DefaultReminderAttribute : public Akonadi::Attribute
{
public:
    explicit DefaultReminderAttribute();

    Attribute *clone() const override;
    void deserialize(const QByteArray &data) override;
    QByteArray serialized() const override;
    QByteArray type() const override;

    void setReminders(const KGAPI2::RemindersList &reminders);
    KCalCore::Alarm::List alarms(KCalCore::Incidence *incidence) const;

private:
    KGAPI2::RemindersList m_reminders;
};

#endif // DEFAULTREMINDERATTRIBUTE_H
