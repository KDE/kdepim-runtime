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
#include "tokenjobs.h"

#include <QNetworkCookie>
#include <QByteArrayMatcher>
#include <QTimeZone>

#include <KIO/Job>
#include <KLocalizedString>
#include <KCharsets>

#include <KCalCore/MemoryCalendar>
#include <KCalCore/ICalFormat>

BirthdayListJob::BirthdayListJob(const QString &identifier, const Akonadi::Collection &collection, QObject *parent)
    : KJob(parent)
    , mCollection(collection)
    , mIdentifier(identifier)
{
}

BirthdayListJob::~BirthdayListJob()
{
}

QVector<Akonadi::Item> BirthdayListJob::items() const
{
    return mItems;
}

KIO::StoredTransferJob *BirthdayListJob::createGetJob(const QUrl &url) const
{
    auto job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    job->setMetaData({ QMap<QString, QString>{
                           { QStringLiteral("cookies"), QStringLiteral("manual") },
                           { QStringLiteral("setcookies"), mCookies }
                       }});
    return job;
}

void BirthdayListJob::emitError(const QString &errorText)
{
    setError(KJob::UserDefinedError);
    setErrorText(errorText);
    emitResult();
}

void BirthdayListJob::start()
{
    auto tokenJob = new GetTokenJob(mIdentifier, parent());
    connect(tokenJob, &GetTokenJob::result,
            this, [this, tokenJob]() {
        if (tokenJob->error()) {
            emitError(tokenJob->errorText());
            return;
        }

        // Convert the cookies into a HTTP Cookie header that we can pass
        // to KIO
        mCookies = QStringLiteral("Cookie: ");
        const auto parsedCookies = QNetworkCookie::parseCookies(tokenJob->cookies());
        for (const auto &cookie : parsedCookies) {
            mCookies += QStringLiteral("%1=%2; ").arg(QString::fromUtf8(cookie.name()),
                                                      QString::fromUtf8(cookie.value()));
        }
        fetchFacebookEventsPage();
    });
    tokenJob->start();
}

void BirthdayListJob::fetchFacebookEventsPage()
{
    auto job = createGetJob(QUrl(QStringLiteral("https://www.facebook.com/events/birthdays")));
    connect(job, &KJob::result,
            this, [this, job]() {
        if (job->error()) {
            emitError(i18n("Failed to retrieve birthday calendar"));
            return;
        }

        auto url = findBirthdayIcalLink(job->data());
        if (url.isEmpty()) {
            emitError(i18n("Failed to retrieve birthday calendar"));
            return;
        }
        // switch webcal scheme for https so we can fetch it with KIO
        url.setScheme(QStringLiteral("https"));
        fetchBirthdayIcal(url);
    });
    job->start();
}

QUrl BirthdayListJob::findBirthdayIcalLink(const QByteArray &data)
{
    // QXmlStreamParser cannot deal with Facebook's broken HTML and refuses
    // to parse it. But we know very well what we are looking for and the
    // address is very unique in the source code, so using QBAMatcher is much more
    // efficient...

    const QByteArrayMatcher matcher("webcal://www.facebook.com/ical/b.php");
    const int start = matcher.indexIn(data);
    if (start == -1) {
        return {};
    }

    const int end = data.indexOf('\"', start);
    if (end == -1) {
        return {};
    }

    const auto str = QString::fromUtf8(data.constData() + start, end - start);
    return QUrl(KCharsets::resolveEntities(str));
}

void BirthdayListJob::fetchBirthdayIcal(const QUrl &url)
{
    auto job = createGetJob(url);
    connect(job, &KJob::result,
            this, [this, job]() {
        if (job->error()) {
            emitError(job->errorText());
            return;
        }

        auto cal = KCalCore::MemoryCalendar::Ptr::create(QTimeZone::systemTimeZone());
        KCalCore::ICalFormat format;
        if (!format.fromRawString(cal, job->data(), false)) {
            emitError(i18n("Failed to parse birthday calendar"));
            return;
        }

        const auto events = cal->events();
        for (const auto &event : events) {
            processEvent(event);
        }

        emitResult();
    });
}

void BirthdayListJob::processEvent(const KCalCore::Event::Ptr &event)
{
    if (Settings::self()->birthdayReminders()) {
        auto alarm = KCalCore::Alarm::Ptr::create(event.data());
        alarm->setDisplayAlarm(event->summary());
        alarm->setStartOffset({ -Settings::self()->birthdayReminderDays(),
                                KCalCore::Duration::Days });
        alarm->setEnabled(true);
        event->addAlarm(alarm);
    }

    const auto uid = event->uid(); // b123456789@facebook.com
    const auto id = uid.mid(1, uid.indexOf(QLatin1Char('@')) - 1); // 123456789

    event->setDescription(QStringLiteral("https://www.facebook.com/%1").arg(id));

    Akonadi::Item item;
    item.setRemoteId(uid);
    item.setGid(uid);
    item.setMimeType(KCalCore::Event::eventMimeType());
    item.setParentCollection(mCollection);
    item.setPayload(event);
    mItems.push_back(item);
}
