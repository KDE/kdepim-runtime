/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "birthdaylistjob.h"
#include "settings.h"

#include <QJsonObject>
#include <QDate>

#include <KCalCore/Event>

#include <KLocalizedString>

BirthdayListJob::BirthdayListJob(const Akonadi::Collection &collection, QObject *parent)
    : ListJob(collection, parent)
{
    setRequest(QStringLiteral("me/friends"),
               { QStringLiteral("id"),
                 QStringLiteral("name"),
                 QStringLiteral("birthday") });
}

BirthdayListJob::~BirthdayListJob()
{
}

Akonadi::Item BirthdayListJob::handleResponse(const QJsonObject &data)
{
    if (!data.contains(QLatin1String("birthday"))) {
        return {};
    }

    auto event = KCalCore::Event::Ptr::create();

    const QString id = data.value(QLatin1String("id")).toString();
    event->setUid(id);
    const QString name = data.value(QLatin1String("name")).toString();
    event->setSummary(i18n("%1's birthday", name));
    event->setDescription(QStringLiteral("https://www.facebook.com/%1").arg(id));

    const QString birthday = data.value(QLatin1String("birthday")).toString();
    const int day = birthday.midRef(3, 2).toInt();
    const int month = birthday.midRef(0, 2).toInt();
    int year = birthday.length() > 6 ? birthday.midRef(6).toInt() : QDate::currentDate().year();
    if (year < 100) { // handle just two-digit years, assume it's 1900's
        year += 1900;
    }
    QDate dt(year, month, day);

    event->setDtStart(KDateTime(dt));
    event->setAllDay(true);
    auto recurrence = event->recurrence();
    recurrence->setYearly(1);

    if (Settings::self()->birthdayReminders()) {
        auto alarm = KCalCore::Alarm::Ptr::create(event.data());
        alarm->setDisplayAlarm(event->summary());
        alarm->setStartOffset({ -Settings::self()->birthdayReminderDays(),
                                KCalCore::Duration::Days });
        alarm->setEnabled(true);
        event->addAlarm(alarm);
    }

    Akonadi::Item item;
    item.setRemoteId(id);
    item.setGid(id);
    item.setMimeType(KCalCore::Event::eventMimeType());
    item.setParentCollection(collection());
    item.setPayload(event);
    return item;
}
