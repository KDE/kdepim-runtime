/*
    SPDX-FileCopyrightText: 2011-2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>

#include <KGAPI/Types>

#include <KCalendarCore/Alarm>

namespace KCalendarCore
{
class Incidence;
}

class DefaultReminderAttribute : public Akonadi::Attribute
{
public:
    explicit DefaultReminderAttribute();

    Attribute *clone() const override;
    void deserialize(const QByteArray &data) override;
    QByteArray serialized() const override;
    QByteArray type() const override;

    void setReminders(const KGAPI2::RemindersList &reminders);
    KCalendarCore::Alarm::List alarms(KCalendarCore::Incidence *incidence) const;

private:
    KGAPI2::RemindersList m_reminders;
};
