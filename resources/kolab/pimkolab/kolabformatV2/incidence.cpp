/*
    This file is part of the kolab resource - the implementation of the
    Kolab storage format. See www.kolab.org for documentation on this.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "incidence.h"
#include "libkolab-version.h"
#include "utils/porting.h"
#include "pimkolab_debug.h"

#include <QList>

#include <kcalcore/journal.h>
#include <kurl.h>

#include <QBitArray>

using namespace KolabV2;

Incidence::Incidence(const QString &tz, const KCalCore::Incidence::Ptr &incidence)
    : KolabBase(tz)
    , mFloatingStatus(Unset)
    , mHasAlarm(false)
    , mPriority(0)
{
    Q_UNUSED(incidence);
}

Incidence::~Incidence()
{
}

void Incidence::setPriority(int priority)
{
    mPriority = priority;
}

int Incidence::priority() const
{
    return mPriority;
}

void Incidence::setSummary(const QString &summary)
{
    mSummary = summary;
}

QString Incidence::summary() const
{
    return mSummary;
}

void Incidence::setLocation(const QString &location)
{
    mLocation = location;
}

QString Incidence::location() const
{
    return mLocation;
}

void Incidence::setOrganizer(const Email &organizer)
{
    mOrganizer = organizer;
}

KolabBase::Email Incidence::organizer() const
{
    return mOrganizer;
}

void Incidence::setStartDate(const KDateTime &startDate)
{
    mStartDate = startDate;
    if (mFloatingStatus == AllDay) {
        qCDebug(PIMKOLAB_LOG) <<"ERROR: Time on start date but no time on the event";
    }
    mFloatingStatus = HasTime;
}

void Incidence::setStartDate(const QDate &startDate)
{
    mStartDate = KDateTime(startDate);
    if (mFloatingStatus == HasTime) {
        qCDebug(PIMKOLAB_LOG) <<"ERROR: No time on start date but time on the event";
    }
    mFloatingStatus = AllDay;
}

void Incidence::setStartDate(const QString &startDate)
{
    if (startDate.length() > 10) {
        // This is a date + time
        setStartDate(stringToDateTime(startDate));
    } else {
        // This is only a date
        setStartDate(stringToDate(startDate));
    }
}

KDateTime Incidence::startDate() const
{
    return mStartDate;
}

void Incidence::setAlarm(float alarm)
{
    mAlarm = alarm;
    mHasAlarm = true;
}

float Incidence::alarm() const
{
    return mAlarm;
}

Incidence::Recurrence Incidence::recurrence() const
{
    return mRecurrence;
}

void Incidence::addAttendee(const Attendee &attendee)
{
    mAttendees.append(attendee);
}

QList<Incidence::Attendee> &Incidence::attendees()
{
    return mAttendees;
}

const QList<Incidence::Attendee> &Incidence::attendees() const
{
    return mAttendees;
}

void Incidence::setInternalUID(const QString &iuid)
{
    mInternalUID = iuid;
}

QString Incidence::internalUID() const
{
    return mInternalUID;
}

bool Incidence::loadAttendeeAttribute(QDomElement &element, Attendee &attendee)
{
    for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment()) {
            continue;
        }
        if (n.isElement()) {
            QDomElement e = n.toElement();
            QString tagName = e.tagName();

            if (tagName == QLatin1String("display-name")) {
                attendee.displayName = e.text();
            } else if (tagName == QLatin1String("smtp-address")) {
                attendee.smtpAddress = e.text();
            } else if (tagName == QLatin1String("status")) {
                attendee.status = e.text();
            } else if (tagName == QLatin1String("request-response")) {
                // This sets reqResp to false, if the text is "false". Otherwise it
                // sets it to true. This means the default setting is true.
                attendee.requestResponse = (e.text().toLower() != QLatin1String("false"));
            } else if (tagName == QLatin1String("invitation-sent")) {
                // Like above, only this defaults to false
                attendee.invitationSent = (e.text().toLower() != QLatin1String("true"));
            } else if (tagName == QLatin1String("role")) {
                attendee.role = e.text();
            } else if (tagName == QLatin1String("delegated-to")) {
                attendee.delegate = e.text();
            } else if (tagName == QLatin1String("delegated-from")) {
                attendee.delegator = e.text();
            } else {
                // TODO: Unhandled tag - save for later storage
                qCDebug(PIMKOLAB_LOG) <<"Warning: Unhandled tag" << e.tagName();
            }
        } else {
            qCDebug(PIMKOLAB_LOG) <<"Node is not a comment or an element???";
        }
    }

    return true;
}

void Incidence::saveAttendeeAttribute(QDomElement &element, const Attendee &attendee) const
{
    QDomElement e = element.ownerDocument().createElement(QStringLiteral("attendee"));
    element.appendChild(e);
    writeString(e, QStringLiteral("display-name"), attendee.displayName);
    writeString(e, QStringLiteral("smtp-address"), attendee.smtpAddress);
    writeString(e, QStringLiteral("status"), attendee.status);
    writeString(e, QStringLiteral("request-response"),
                (attendee.requestResponse ? QStringLiteral("true") : QStringLiteral("false")));
    writeString(e, QStringLiteral("invitation-sent"),
                (attendee.invitationSent ? QStringLiteral("true") : QStringLiteral("false")));
    writeString(e, QStringLiteral("role"), attendee.role);
    writeString(e, QStringLiteral("delegated-to"), attendee.delegate);
    writeString(e, QStringLiteral("delegated-from"), attendee.delegator);
}

void Incidence::saveAttendees(QDomElement &element) const
{
    foreach (const Attendee &attendee, mAttendees) {
        saveAttendeeAttribute(element, attendee);
    }
}

void Incidence::saveAttachments(QDomElement &element) const
{
    foreach (KCalCore::Attachment::Ptr a, mAttachments) {
        if (a->isUri()) {
            writeString(element, QStringLiteral("link-attachment"), a->uri());
        } else if (a->isBinary()) {
            writeString(element, QStringLiteral("inline-attachment"), a->label());
        }
    }
}

void Incidence::saveAlarms(QDomElement &element) const
{
    if (mAlarms.isEmpty()) {
        return;
    }

    QDomElement list = element.ownerDocument().createElement(QStringLiteral("advanced-alarms"));
    element.appendChild(list);
    foreach (KCalCore::Alarm::Ptr a, mAlarms) {
        QDomElement e = list.ownerDocument().createElement(QStringLiteral("alarm"));
        list.appendChild(e);

        writeString(e, QStringLiteral("enabled"), a->enabled() ? QStringLiteral("1") : QStringLiteral("0"));
        if (a->hasStartOffset()) {
            writeString(e, QStringLiteral("start-offset"), QString::number(a->startOffset().asSeconds()/60));
        }
        if (a->hasEndOffset()) {
            writeString(e, QStringLiteral("end-offset"), QString::number(a->endOffset().asSeconds()/60));
        }
        if (a->repeatCount()) {
            writeString(e, QStringLiteral("repeat-count"), QString::number(a->repeatCount()));
            writeString(e, QStringLiteral("repeat-interval"), QString::number(a->snoozeTime().asSeconds()));
        }

        switch (a->type()) {
        case KCalCore::Alarm::Invalid:
            break;
        case KCalCore::Alarm::Display:
            e.setAttribute(QStringLiteral("type"), QStringLiteral("display"));
            writeString(e, QStringLiteral("text"), a->text());
            break;
        case KCalCore::Alarm::Procedure:
            e.setAttribute(QStringLiteral("type"), QStringLiteral("procedure"));
            writeString(e, QStringLiteral("program"), a->programFile());
            writeString(e, QStringLiteral("arguments"), a->programArguments());
            break;
        case KCalCore::Alarm::Email:
        {
            e.setAttribute(QStringLiteral("type"), QStringLiteral("email"));
            QDomElement addresses = e.ownerDocument().createElement(QStringLiteral("addresses"));
            e.appendChild(addresses);
            foreach (const KCalCore::Person::Ptr &person, a->mailAddresses()) {
                writeString(addresses, QStringLiteral("address"), person->fullName());
            }
            writeString(e, QStringLiteral("subject"), a->mailSubject());
            writeString(e, QStringLiteral("mail-text"), a->mailText());
            QDomElement attachments = e.ownerDocument().createElement(QStringLiteral("attachments"));
            e.appendChild(attachments);
            foreach (const QString &attachment, a->mailAttachments()) {
                writeString(attachments, QStringLiteral("attachment"), attachment);
            }
            break;
        }
        case KCalCore::Alarm::Audio:
            e.setAttribute(QStringLiteral("type"), QStringLiteral("audio"));
            writeString(e, QStringLiteral("file"), a->audioFile());
            break;
        default:
            qCWarning(PIMKOLAB_LOG) << "Unhandled alarm type:" << a->type();
            break;
        }
    }
}

void Incidence::saveRecurrence(QDomElement &element) const
{
    QDomElement e = element.ownerDocument().createElement(QStringLiteral("recurrence"));
    element.appendChild(e);
    e.setAttribute(QStringLiteral("cycle"), mRecurrence.cycle);
    if (!mRecurrence.type.isEmpty()) {
        e.setAttribute(QStringLiteral("type"), mRecurrence.type);
    }
    writeString(e, QStringLiteral("interval"), QString::number(mRecurrence.interval));
    foreach (const QString &recurrence, mRecurrence.days) {
        writeString(e, QStringLiteral("day"), recurrence);
    }
    if (!mRecurrence.dayNumber.isEmpty()) {
        writeString(e, QStringLiteral("daynumber"), mRecurrence.dayNumber);
    }
    if (!mRecurrence.month.isEmpty()) {
        writeString(e, QStringLiteral("month"), mRecurrence.month);
    }
    if (!mRecurrence.rangeType.isEmpty()) {
        QDomElement range = element.ownerDocument().createElement(QStringLiteral("range"));
        e.appendChild(range);
        range.setAttribute(QStringLiteral("type"), mRecurrence.rangeType);
        QDomText t = element.ownerDocument().createTextNode(mRecurrence.range);
        range.appendChild(t);
    }
    foreach (const QDate &date, mRecurrence.exclusions) {
        writeString(e, QStringLiteral("exclusion"), dateToString(date));
    }
}

void Incidence::loadRecurrence(const QDomElement &element)
{
    mRecurrence.interval = 0;
    mRecurrence.cycle = element.attribute(QStringLiteral("cycle"));
    mRecurrence.type = element.attribute(QStringLiteral("type"));
    for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment()) {
            continue;
        }
        if (n.isElement()) {
            QDomElement e = n.toElement();
            QString tagName = e.tagName();
            if (tagName == QLatin1String("interval")) {
                //kolab/issue4229, sometimes  the interval value can be empty
                if (e.text().isEmpty() || e.text().toInt() <= 0) {
                    mRecurrence.interval = 1;
                } else {
                    mRecurrence.interval = e.text().toInt();
                }
            } else if (tagName == QLatin1String("day")) { // can be present multiple times
                mRecurrence.days.append(e.text());
            } else if (tagName == QLatin1String("daynumber")) {
                mRecurrence.dayNumber = e.text();
            } else if (tagName == QLatin1String("month")) {
                mRecurrence.month = e.text();
            } else if (tagName == QLatin1String("range")) {
                mRecurrence.rangeType = e.attribute(QStringLiteral("type"));
                mRecurrence.range = e.text();
            } else if (tagName == QLatin1String("exclusion")) {
                mRecurrence.exclusions.append(stringToDate(e.text()));
            } else {
                // TODO: Unhandled tag - save for later storage
                qCDebug(PIMKOLAB_LOG) <<"Warning: Unhandled tag" << e.tagName();
            }
        }
    }
}

static void loadAddressesHelper(const QDomElement &element, const KCalCore::Alarm::Ptr &a)
{
    for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment()) {
            continue;
        }
        if (n.isElement()) {
            QDomElement e = n.toElement();
            QString tagName = e.tagName();

            if (tagName == QLatin1String("address")) {
                a->addMailAddress(KCalCore::Person::fromFullName(e.text()));
            } else {
                qCWarning(PIMKOLAB_LOG) << "Unhandled tag" << tagName;
            }
        }
    }
}

static void loadAttachmentsHelper(const QDomElement &element, const KCalCore::Alarm::Ptr &a)
{
    for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment()) {
            continue;
        }
        if (n.isElement()) {
            QDomElement e = n.toElement();
            QString tagName = e.tagName();

            if (tagName == QLatin1String("attachment")) {
                a->addMailAttachment(e.text());
            } else {
                qCWarning(PIMKOLAB_LOG) << "Unhandled tag" << tagName;
            }
        }
    }
}

static void loadAlarmHelper(const QDomElement &element, const KCalCore::Alarm::Ptr &a)
{
    for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment()) {
            continue;
        }
        if (n.isElement()) {
            QDomElement e = n.toElement();
            QString tagName = e.tagName();

            if (tagName == QLatin1String("start-offset")) {
                a->setStartOffset(e.text().toInt()*60);
            } else if (tagName == QLatin1String("end-offset")) {
                a->setEndOffset(e.text().toInt()*60);
            } else if (tagName == QLatin1String("repeat-count")) {
                a->setRepeatCount(e.text().toInt());
            } else if (tagName == QLatin1String("repeat-interval")) {
                a->setSnoozeTime(e.text().toInt());
            } else if (tagName == QLatin1String("text")) {
                a->setText(e.text());
            } else if (tagName == QLatin1String("program")) {
                a->setProgramFile(e.text());
            } else if (tagName == QLatin1String("arguments")) {
                a->setProgramArguments(e.text());
            } else if (tagName == QLatin1String("addresses")) {
                loadAddressesHelper(e, a);
            } else if (tagName == QLatin1String("subject")) {
                a->setMailSubject(e.text());
            } else if (tagName == QLatin1String("mail-text")) {
                a->setMailText(e.text());
            } else if (tagName == QLatin1String("attachments")) {
                loadAttachmentsHelper(e, a);
            } else if (tagName == QLatin1String("file")) {
                a->setAudioFile(e.text());
            } else if (tagName == QLatin1String("enabled")) {
                a->setEnabled(e.text().toInt() != 0);
            } else {
                qCWarning(PIMKOLAB_LOG) << "Unhandled tag" << tagName;
            }
        }
    }
}

void Incidence::loadAlarms(const QDomElement &element)
{
    for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment()) {
            continue;
        }
        if (n.isElement()) {
            QDomElement e = n.toElement();
            QString tagName = e.tagName();

            if (tagName == QLatin1String("alarm")) {
                KCalCore::Alarm::Ptr a = KCalCore::Alarm::Ptr(new KCalCore::Alarm(nullptr));
                a->setEnabled(true); // default to enabled, unless some XML attribute says otherwise.
                QString type = e.attribute(QStringLiteral("type"));
                if (type == QLatin1String("display")) {
                    a->setType(KCalCore::Alarm::Display);
                } else if (type == QLatin1String("procedure")) {
                    a->setType(KCalCore::Alarm::Procedure);
                } else if (type == QLatin1String("email")) {
                    a->setType(KCalCore::Alarm::Email);
                } else if (type == QLatin1String("audio")) {
                    a->setType(KCalCore::Alarm::Audio);
                } else {
                    qCWarning(PIMKOLAB_LOG) << "Unhandled alarm type:" << type;
                }

                loadAlarmHelper(e, a);
                mAlarms << a;
            } else {
                qCWarning(PIMKOLAB_LOG) << "Unhandled tag" << tagName;
            }
        }
    }
}

bool Incidence::loadAttribute(QDomElement &element)
{
    QString tagName = element.tagName();

    if (tagName == QLatin1String("priority")) {
        bool ok;
        int p = element.text().toInt(&ok);
        if (!ok || p < 1 || p > 9) {
            qCWarning(PIMKOLAB_LOG) << "Invalid \"priority\" value:" << element.text();
        } else {
            setPriority(p);
        }
    } else if (tagName == QLatin1String("x-kcal-priority")) { //for backwards compat
        bool ok;
        int p = element.text().toInt(&ok);
        if (!ok || p < 0 || p > 9) {
            qCWarning(PIMKOLAB_LOG) << "Invalid \"x-kcal-priority\" value:" << element.text();
        } else {
            if (priority() == 0) {
                setPriority(p);
            }
        }
    } else if (tagName == QLatin1String("summary")) {
        setSummary(element.text());
    } else if (tagName == QLatin1String("location")) {
        setLocation(element.text());
    } else if (tagName == QLatin1String("organizer")) {
        Email email;
        if (loadEmailAttribute(element, email)) {
            setOrganizer(email);
            return true;
        } else {
            return false;
        }
    } else if (tagName == QLatin1String("start-date")) {
        setStartDate(element.text());
    } else if (tagName == QLatin1String("recurrence")) {
        loadRecurrence(element);
    } else if (tagName == QLatin1String("attendee")) {
        Attendee attendee;
        if (loadAttendeeAttribute(element, attendee)) {
            addAttendee(attendee);
            return true;
        } else {
            return false;
        }
    } else if (tagName == QLatin1String("link-attachment")) {
        mAttachments.push_back(KCalCore::Attachment::Ptr(new KCalCore::Attachment(element.text())));
    } else if (tagName == QLatin1String("alarm")) {
        // Alarms should be minutes before. Libkcal uses event time + alarm time
        setAlarm(-element.text().toInt());
    } else if (tagName == QLatin1String("advanced-alarms")) {
        loadAlarms(element);
    } else if (tagName == QLatin1String("x-kde-internaluid")) {
        setInternalUID(element.text());
    } else if (tagName == QLatin1String("x-custom")) {
        loadCustomAttributes(element);
    } else if (tagName == QLatin1String("inline-attachment")) {
        // we handle that separately later on, so no need to create a KolabUnhandled entry for it
    } else {
        bool ok = KolabBase::loadAttribute(element);
        if (!ok) {
            // Unhandled tag - save for later storage
            qCDebug(PIMKOLAB_LOG) <<"Saving unhandled tag" << element.tagName();
            Custom c;
            c.key = QByteArray("X-KDE-KolabUnhandled-") + element.tagName().toLatin1();
            c.value = element.text();
            mCustomList.append(c);
        }
    }
    // We handled this
    return true;
}

bool Incidence::saveAttributes(QDomElement &element) const
{
    // Save the base class elements
    KolabBase::saveAttributes(element);

    if (priority() != 0) {
        writeString(element, QStringLiteral("priority"), QString::number(priority()));
    }

    if (mFloatingStatus == HasTime) {
        writeString(element, QStringLiteral("start-date"), dateTimeToString(startDate()));
    } else {
        writeString(element, QStringLiteral("start-date"), dateToString(startDate().date()));
    }
    writeString(element, QStringLiteral("summary"), summary());
    writeString(element, QStringLiteral("location"), location());
    saveEmailAttribute(element, organizer(), QStringLiteral("organizer"));
    if (!mRecurrence.cycle.isEmpty()) {
        saveRecurrence(element);
    }
    saveAttendees(element);
    saveAttachments(element);
    if (mHasAlarm) {
        // Alarms should be minutes before. Libkcal uses event time + alarm time
        int alarmTime = qRound(-alarm());
        writeString(element, QStringLiteral("alarm"), QString::number(alarmTime));
    }
    saveAlarms(element);
    writeString(element, QStringLiteral("x-kde-internaluid"), internalUID());
    saveCustomAttributes(element);
    return true;
}

void Incidence::saveCustomAttributes(QDomElement &element) const
{
    foreach (const Custom &custom, mCustomList) {
        QString key(custom.key);
        Q_ASSERT(!key.isEmpty());
        if (key.startsWith(QLatin1String("X-KDE-KolabUnhandled-"))) {
            key = key.mid(strlen("X-KDE-KolabUnhandled-"));
            writeString(element, key, custom.value);
        } else {
            // Let's use attributes so that other tag-preserving-code doesn't need sub-elements
            QDomElement e = element.ownerDocument().createElement(QStringLiteral("x-custom"));
            element.appendChild(e);
            e.setAttribute(QStringLiteral("key"), key);
            e.setAttribute(QStringLiteral("value"), custom.value);
        }
    }
}

void Incidence::loadCustomAttributes(QDomElement &element)
{
    Custom custom;
    custom.key = element.attribute(QStringLiteral("key")).toLatin1();
    custom.value = element.attribute(QStringLiteral("value"));
    mCustomList.append(custom);
}

static KCalCore::Attendee::PartStat attendeeStringToStatus(const QString &s)
{
    if (s == QLatin1String("none")) {
        return KCalCore::Attendee::NeedsAction;
    }
    if (s == QLatin1String("tentative")) {
        return KCalCore::Attendee::Tentative;
    }
    if (s == QLatin1String("declined")) {
        return KCalCore::Attendee::Declined;
    }
    if (s == QLatin1String("delegated")) {
        return KCalCore::Attendee::Delegated;
    }

    // Default:
    return KCalCore::Attendee::Accepted;
}

static QString attendeeStatusToString(KCalCore::Attendee::PartStat status)
{
    switch (status) {
    case KCalCore::Attendee::NeedsAction:
        return QStringLiteral("none");
    case KCalCore::Attendee::Accepted:
        return QStringLiteral("accepted");
    case KCalCore::Attendee::Declined:
        return QStringLiteral("declined");
    case KCalCore::Attendee::Tentative:
        return QStringLiteral("tentative");
    case KCalCore::Attendee::Delegated:
        return QStringLiteral("delegated");
    case KCalCore::Attendee::Completed:
    case KCalCore::Attendee::InProcess:
        // These don't have any meaning in the Kolab format, so just use:
        return QStringLiteral("accepted");
    default:
        // Default for the case that there are more added later:
        return QStringLiteral("accepted");
    }
}

static KCalCore::Attendee::Role attendeeStringToRole(const QString &s)
{
    if (s == QLatin1String("optional")) {
        return KCalCore::Attendee::OptParticipant;
    }
    if (s == QLatin1String("resource")) {
        return KCalCore::Attendee::NonParticipant;
    }
    return KCalCore::Attendee::ReqParticipant;
}

static QString attendeeRoleToString(KCalCore::Attendee::Role role)
{
    switch (role) {
    case KCalCore::Attendee::ReqParticipant:
        return QStringLiteral("required");
    case KCalCore::Attendee::OptParticipant:
        return QStringLiteral("optional");
    case KCalCore::Attendee::Chair:
        // We don't have the notion of chair, so use
        return QStringLiteral("required");
    case KCalCore::Attendee::NonParticipant:
        // In Kolab, a non-participant is a resource
        return QStringLiteral("resource");
    }

    // Default for the case that there are more added later:
    return QStringLiteral("required");
}

static const char *s_weekDayName[] = {
    "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"
};

static const char *s_monthName[] = {
    "january", "february", "march", "april", "may", "june", "july",
    "august", "september", "october", "november", "december"
};

void Incidence::setRecurrence(KCalCore::Recurrence *recur)
{
    mRecurrence.interval = recur->frequency();
    switch (recur->recurrenceType()) {
    case KCalCore::Recurrence::rMinutely: // Not handled by the kolab XML
        mRecurrence.cycle = QStringLiteral("minutely");
        break;
    case KCalCore::Recurrence::rHourly: // Not handled by the kolab XML
        mRecurrence.cycle = QStringLiteral("hourly");
        break;
    case KCalCore::Recurrence::rDaily:
        mRecurrence.cycle = QStringLiteral("daily");
        break;
    case KCalCore::Recurrence::rWeekly: // every X weeks
        mRecurrence.cycle = QStringLiteral("weekly");
        {
            QBitArray arr = recur->days();
            for (int idx = 0; idx < 7; ++idx) {
                if (arr.testBit(idx)) {
                    mRecurrence.days.append(s_weekDayName[idx]);
                }
            }
        }
        break;
    case KCalCore::Recurrence::rMonthlyPos:
    {
        mRecurrence.cycle = QStringLiteral("monthly");
        mRecurrence.type = QStringLiteral("weekday");
        QList<KCalCore::RecurrenceRule::WDayPos> monthPositions = recur->monthPositions();
        if (!monthPositions.isEmpty()) {
            KCalCore::RecurrenceRule::WDayPos monthPos = monthPositions.first();
            // TODO: Handle multiple days in the same week
            mRecurrence.dayNumber = QString::number(monthPos.pos());
            mRecurrence.days.append(s_weekDayName[ monthPos.day()-1 ]);
            // Not (properly) handled(?): monthPos.negative (nth days before end of month)
        }
        break;
    }
    case KCalCore::Recurrence::rMonthlyDay:
    {
        mRecurrence.cycle = QStringLiteral("monthly");
        mRecurrence.type = QStringLiteral("daynumber");
        QList<int> monthDays = recur->monthDays();
        // ####### Kolab XML limitation: only the first month day is used
        if (!monthDays.isEmpty()) {
            mRecurrence.dayNumber = QString::number(monthDays.first());
        }
        break;
    }
    case KCalCore::Recurrence::rYearlyMonth: // (day n of Month Y)
    {
        mRecurrence.cycle = QStringLiteral("yearly");
        mRecurrence.type = QStringLiteral("monthday");
        QList<int> rmd = recur->yearDates();
        int day = !rmd.isEmpty() ? rmd.first() : recur->startDate().day();
        mRecurrence.dayNumber = QString::number(day);
        QList<int> months = recur->yearMonths();
        if (!months.isEmpty()) {
            mRecurrence.month = s_monthName[ months.first() - 1 ]; // #### Kolab XML limitation: only one month specified
        }
        break;
    }
    case KCalCore::Recurrence::rYearlyDay: // YearlyDay (day N of the year). Not supported by Outlook
        mRecurrence.cycle = QStringLiteral("yearly");
        mRecurrence.type = QStringLiteral("yearday");
        mRecurrence.dayNumber = QString::number(recur->yearDays().first());
        break;
    case KCalCore::Recurrence::rYearlyPos: // (weekday X of week N of month Y)
        mRecurrence.cycle = QStringLiteral("yearly");
        mRecurrence.type = QStringLiteral("weekday");
        QList<int> months = recur->yearMonths();
        if (!months.isEmpty()) {
            mRecurrence.month = s_monthName[ months.first() - 1 ]; // #### Kolab XML limitation: only one month specified
        }
        QList<KCalCore::RecurrenceRule::WDayPos> monthPositions = recur->yearPositions();
        if (!monthPositions.isEmpty()) {
            KCalCore::RecurrenceRule::WDayPos monthPos = monthPositions.first();
            // TODO: Handle multiple days in the same week
            mRecurrence.dayNumber = QString::number(monthPos.pos());
            mRecurrence.days.append(s_weekDayName[ monthPos.day()-1 ]);

            //mRecurrence.dayNumber = QString::number( *recur->yearNums().getFirst() );
            // Not handled: monthPos.negative (nth days before end of month)
        }
        break;
    }
    int howMany = recur->duration();
    if (howMany > 0) {
        mRecurrence.rangeType = QStringLiteral("number");
        mRecurrence.range = QString::number(howMany);
    } else if (howMany == 0) {
        mRecurrence.rangeType = QStringLiteral("date");
        mRecurrence.range = dateToString(recur->endDate());
    } else {
        mRecurrence.rangeType = QStringLiteral("none");
    }
}

void Incidence::setFields(const KCalCore::Incidence::Ptr &incidence)
{
    KolabBase::setFields(incidence);

    setPriority(incidence->priority());
    if (incidence->allDay()) {
        // This is a all-day event. Don't timezone move this one
        mFloatingStatus = AllDay;
        setStartDate(incidence->dtStart().date());
    } else {
        mFloatingStatus = HasTime;
        setStartDate(localToUTC(Porting::q2k(incidence->dtStart())));
    }

    setSummary(incidence->summary());
    setLocation(incidence->location());

    // Alarm
    mHasAlarm = false; // Will be set to true, if we actually have one
    if (incidence->hasEnabledAlarms()) {
        const KCalCore::Alarm::List &alarms = incidence->alarms();
        if (!alarms.isEmpty()) {
            const KCalCore::Alarm::Ptr alarm = alarms.first();
            if (alarm->hasStartOffset()) {
                int dur = alarm->startOffset().asSeconds();
                setAlarm((float)dur / 60.0);
            }
        }
    }

    if (incidence->organizer()) {
        Email org(incidence->organizer()->name(), incidence->organizer()->email());
        setOrganizer(org);
    }

    // Attendees:
    const KCalCore::Attendee::List attendees = incidence->attendees();
    foreach (const KCalCore::Attendee::Ptr &kcalAttendee, attendees) {
        Attendee attendee;

        attendee.displayName = kcalAttendee->name();
        attendee.smtpAddress = kcalAttendee->email();
        attendee.status = attendeeStatusToString(kcalAttendee->status());
        attendee.requestResponse = kcalAttendee->RSVP();
        // TODO: KCalCore::Attendee::mFlag is not accessible
        // attendee.invitationSent = kcalAttendee->mFlag;
        // DF: Hmm? mFlag is set to true and never used at all.... Did you mean another field?
        attendee.role = attendeeRoleToString(kcalAttendee->role());
        attendee.delegate = kcalAttendee->delegate();
        attendee.delegator = kcalAttendee->delegator();

        addAttendee(attendee);
    }

    mAttachments.clear();

    // Attachments
    const KCalCore::Attachment::List attachments = incidence->attachments();
    mAttachments.reserve(attachments.size());
    for (const KCalCore::Attachment::Ptr &a : attachments) {
        mAttachments.push_back(a);
    }

    mAlarms.clear();

    // Alarms
    const KCalCore::Alarm::List alarms = incidence->alarms();
    mAlarms.reserve(alarms.count());
    for (const KCalCore::Alarm::Ptr &a : alarms) {
        mAlarms.push_back(a);
    }

    if (incidence->recurs()) {
        setRecurrence(incidence->recurrence());
        mRecurrence.exclusions = incidence->recurrence()->exDates();
    }

    // Handle the scheduling ID
    if (incidence->schedulingID() == incidence->uid()) {
        // There is no scheduling ID
        setInternalUID(QString());  //krazy:exclude=nullstrassign for old broken gcc
    } else {
        // We've internally been using a different uid, so save that as the
        // temporary (internal) uid and restore the original uid, the one that
        // is used in the folder and the outside world
        setUid(incidence->schedulingID());
        setInternalUID(incidence->uid());
    }

    // Unhandled tags and other custom properties (see libkcal/customproperties.h)
    const QMap<QByteArray, QString> map = incidence->customProperties();
    QMap<QByteArray, QString>::ConstIterator cit = map.begin();
    QMap<QByteArray, QString>::ConstIterator cend = map.end();
    for (; cit != cend; ++cit) {
        Custom c;
        c.key = cit.key();
        c.value = cit.value();
        mCustomList.append(c);
    }
}

static QBitArray daysListToBitArray(const QStringList &days)
{
    QBitArray arr(7);
    arr.fill(false);
    foreach (const QString &day, days) {
        for (int i = 0; i < 7; ++i) {
            if (day == s_weekDayName[i]) {
                arr.setBit(i, true);
            }
        }
    }
    return arr;
}

void Incidence::saveTo(const KCalCore::Incidence::Ptr &incidence)
{
    KolabBase::saveTo(incidence);

    incidence->setPriority(priority());
    if (mFloatingStatus == AllDay) {
        // This is an all-day event. Don't timezone move this one
        incidence->setDtStart(Porting::k2q(startDate()));
        incidence->setAllDay(true);
    } else {
        incidence->setDtStart(Porting::k2q(utcToLocal(startDate())));
        incidence->setAllDay(false);
    }

    incidence->setSummary(summary());
    incidence->setLocation(location());

    if (mHasAlarm && mAlarms.isEmpty()) {
        KCalCore::Alarm::Ptr alarm = incidence->newAlarm();
        alarm->setStartOffset(qRound(mAlarm * 60.0));
        alarm->setEnabled(true);
        alarm->setType(KCalCore::Alarm::Display);
    } else if (!mAlarms.isEmpty()) {
        foreach (KCalCore::Alarm::Ptr a, mAlarms) {
            a->setParent(incidence.data());
            incidence->addAlarm(a);
        }
    }

    if (organizer().displayName.isEmpty()) {
        incidence->setOrganizer(organizer().smtpAddress);
    } else {
        incidence->setOrganizer(organizer().displayName + QLatin1Char('<')
                                + organizer().smtpAddress + QLatin1Char('>'));
    }

    incidence->clearAttendees();
    foreach (const Attendee &attendee, mAttendees) {
        KCalCore::Attendee::PartStat status = attendeeStringToStatus(attendee.status);
        KCalCore::Attendee::Role role = attendeeStringToRole(attendee.role);
        KCalCore::Attendee::Ptr a(new KCalCore::Attendee(attendee.displayName,
                                                         attendee.smtpAddress,
                                                         attendee.requestResponse,
                                                         status, role));
        a->setDelegate(attendee.delegate);
        a->setDelegator(attendee.delegator);
        incidence->addAttendee(a);
    }

    incidence->clearAttachments();
    foreach (const KCalCore::Attachment::Ptr &a, mAttachments) {
        // TODO should we copy?
        incidence->addAttachment(a);
    }

    if (!mRecurrence.cycle.isEmpty()) {
        KCalCore::Recurrence *recur = incidence->recurrence(); // yeah, this creates it
        // done below recur->setFrequency( mRecurrence.interval );
        if (mRecurrence.cycle == QLatin1String("minutely")) {
            recur->setMinutely(mRecurrence.interval);
        } else if (mRecurrence.cycle == QLatin1String("hourly")) {
            recur->setHourly(mRecurrence.interval);
        } else if (mRecurrence.cycle == QLatin1String("daily")) {
            recur->setDaily(mRecurrence.interval);
        } else if (mRecurrence.cycle == QLatin1String("weekly")) {
            QBitArray rDays = daysListToBitArray(mRecurrence.days);
            recur->setWeekly(mRecurrence.interval, rDays);
        } else if (mRecurrence.cycle == QLatin1String("monthly")) {
            recur->setMonthly(mRecurrence.interval);
            if (mRecurrence.type == QLatin1String("weekday")) {
                recur->addMonthlyPos(mRecurrence.dayNumber.toInt(), daysListToBitArray(mRecurrence.days));
            } else if (mRecurrence.type == QLatin1String("daynumber")) {
                recur->addMonthlyDate(mRecurrence.dayNumber.toInt());
            } else {
                qCWarning(PIMKOLAB_LOG) <<"Unhandled monthly recurrence type" << mRecurrence.type;
            }
        } else if (mRecurrence.cycle == QLatin1String("yearly")) {
            recur->setYearly(mRecurrence.interval);
            if (mRecurrence.type == QLatin1String("monthday")) {
                recur->addYearlyDate(mRecurrence.dayNumber.toInt());
                for (int i = 0; i < 12; ++i) {
                    if (s_monthName[ i ] == mRecurrence.month) {
                        recur->addYearlyMonth(i+1);
                    }
                }
            } else if (mRecurrence.type == QLatin1String("yearday")) {
                recur->addYearlyDay(mRecurrence.dayNumber.toInt());
            } else if (mRecurrence.type == QLatin1String("weekday")) {
                for (int i = 0; i < 12; ++i) {
                    if (s_monthName[ i ] == mRecurrence.month) {
                        recur->addYearlyMonth(i+1);
                    }
                }
                recur->addYearlyPos(mRecurrence.dayNumber.toInt(), daysListToBitArray(mRecurrence.days));
            } else {
                qCWarning(PIMKOLAB_LOG) <<"Unhandled yearly recurrence type" << mRecurrence.type;
            }
        } else {
            qCWarning(PIMKOLAB_LOG) <<"Unhandled recurrence cycle" << mRecurrence.cycle;
        }

        if (mRecurrence.rangeType == QLatin1String("number")) {
            recur->setDuration(mRecurrence.range.toInt());
        } else if (mRecurrence.rangeType == QLatin1String("date")) {
            recur->setEndDate(stringToDate(mRecurrence.range));
        } // "none" is default since tje set*ly methods set infinite recurrence

        incidence->recurrence()->setExDates(mRecurrence.exclusions);
    }
    /* If we've stored a uid to be used internally instead of the real one
     * (to deal with duplicates of events in different folders) before, then
     * restore it, so it does not change. Keep the original uid around for
     * scheduling purposes. */
    if (!internalUID().isEmpty()) {
        incidence->setUid(internalUID());
        incidence->setSchedulingID(uid());
    }

    foreach (const Custom &custom, mCustomList) {
        incidence->setNonKDECustomProperty(custom.key, custom.value);
    }
}

QString Incidence::productID() const
{
    return QStringLiteral("%1, Kolab resource").arg(LIBKOLAB_LIB_VERSION_STRING);
}

// Unhandled KCalCore::Incidence fields:
// revision, status (unused), attendee.uid,
// mComments, mReadOnly
