/*

    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "imip.h"
#include "pimkolab_debug.h"

#include <kcalutils/incidenceformatter.h>
#include <KEmailAddress>
#include <kmime/kmime_message.h>
/*
 * The code in here is copy paste work from kdepim/calendarsupport.
 *
 * We need to refactor the code there and move the relevant parts to kdepimlibs to make it reusable.
 *
 *
 */

//From MailClient::send
KMime::Message::Ptr createMessage(const QString &from, const QString &_to, const QString &cc, const QString &subject, const QString &body, bool hidden, bool bccMe, const QString &attachment /*, const QString &mailTransport */)
{
    Q_UNUSED(hidden);

    const bool outlookConformInvitation = false;
    QString userAgent = QStringLiteral("libkolab");

    // We must have a recipients list for most MUAs. Thus, if the 'to' list
    // is empty simply use the 'from' address as the recipient.
    QString to = _to;
    if (to.isEmpty()) {
        to = from;
    }
    qCDebug(PIMKOLAB_LOG) << "\nFrom:" << from
                          << "\nTo:" << to
                          << "\nCC:" << cc
                          << "\nSubject:" << subject << "\nBody: \n" << body
                          << "\nAttachment:\n" << attachment
        /*<< "\nmailTransport: " << mailTransport*/;

    // Now build the message we like to send. The message KMime::Message::Ptr instance
    // will be the root message that has 2 additional message. The body itself and
    // the attached cal.ics calendar file.
    KMime::Message::Ptr message = KMime::Message::Ptr(new KMime::Message);
    message->contentTransferEncoding()->clear(); // 7Bit, decoded.

    // Set the headers
    message->userAgent()->fromUnicodeString(userAgent, "utf-8");
    message->from()->fromUnicodeString(from, "utf-8");
    message->to()->fromUnicodeString(to, "utf-8");
    message->cc()->fromUnicodeString(cc, "utf-8");
    if (bccMe) {
        message->bcc()->fromUnicodeString(from, "utf-8"); //from==me, right?
    }
    message->date()->setDateTime(QDateTime::currentDateTime());
    message->subject()->fromUnicodeString(subject, "utf-8");

    if (outlookConformInvitation) {
        message->contentType()->setMimeType("text/calendar");
        message->contentType()->setCharset("utf-8");
        message->contentType()->setName(QStringLiteral("cal.ics"), "utf-8");
        message->contentType()->setParameter(QStringLiteral("method"), QStringLiteral("request"));

        if (!attachment.isEmpty()) {
            KMime::Headers::ContentDisposition *disposition
                = new KMime::Headers::ContentDisposition();
            disposition->setDisposition(KMime::Headers::CDinline);
            message->setHeader(disposition);
            message->contentTransferEncoding()->setEncoding(KMime::Headers::CEquPr);
            message->setBody(KMime::CRLFtoLF(attachment.toUtf8()));
        }
    } else {
        // We need to set following 4 lines by hand else KMime::Content::addContent
        // will create a new Content instance for us to attach the main message
        // what we don't need cause we already have the main message instance where
        // 2 additional messages are attached.
        KMime::Headers::ContentType *ct = message->contentType();
        ct->setMimeType("multipart/mixed");
        ct->setBoundary(KMime::multiPartBoundary());
        ct->setCategory(KMime::Headers::CCcontainer);

        // Set the first multipart, the body message.
        KMime::Content *bodyMessage = new KMime::Content;
        KMime::Headers::ContentDisposition *bodyDisposition
            = new KMime::Headers::ContentDisposition();
        bodyDisposition->setDisposition(KMime::Headers::CDinline);
        bodyMessage->contentType()->setMimeType("text/plain");
        bodyMessage->contentType()->setCharset("utf-8");
        bodyMessage->contentTransferEncoding()->setEncoding(KMime::Headers::CEquPr);
        bodyMessage->setBody(KMime::CRLFtoLF(body.toUtf8()));
        message->addContent(bodyMessage);

        // Set the sedcond multipart, the attachment.
        if (!attachment.isEmpty()) {
            KMime::Content *attachMessage = new KMime::Content;
            KMime::Headers::ContentDisposition *attachDisposition
                = new KMime::Headers::ContentDisposition();
            attachDisposition->setDisposition(KMime::Headers::CDattachment);
            attachMessage->contentType()->setMimeType("text/calendar");
            attachMessage->contentType()->setCharset("utf-8");
            attachMessage->contentType()->setName(QStringLiteral("cal.ics"), "utf-8");
            attachMessage->contentType()->setParameter(QStringLiteral("method"),
                                                       QStringLiteral("request"));
            attachMessage->setHeader(attachDisposition);
            attachMessage->contentTransferEncoding()->setEncoding(KMime::Headers::CEquPr);
            attachMessage->setBody(KMime::CRLFtoLF(attachment.toUtf8()));
            message->addContent(attachMessage);
        }
    }

    // Job done, attach the both multiparts and assemble the message.
    message->assemble();
    return message;
}

//From MailClient::mailAttendees
QByteArray mailAttendees(const KCalCore::IncidenceBase::Ptr &incidence,
//                                 const KPIMIdentities::Identity &identity,
                         bool bccMe, const QString &attachment
                         /*const QString &mailTransport */)
{
    KCalCore::Attendee::List attendees = incidence->attendees();
    if (attendees.isEmpty()) {
        qCWarning(PIMKOLAB_LOG) << "There are no attendees to e-mail";
        return QByteArray();
    }

    const QString from = incidence->organizer().fullName();
    const QString organizerEmail = incidence->organizer().email();

    QStringList toList;
    QStringList ccList;
    const int numberOfAttendees(attendees.count());
    for (int i = 0; i < numberOfAttendees; ++i) {
        KCalCore::Attendee::Ptr a = attendees.at(i);

        const QString email = a->email();
        if (email.isEmpty()) {
            continue;
        }

        // In case we (as one of our identities) are the organizer we are sending
        // this mail. We could also have added ourselves as an attendee, in which
        // case we don't want to send ourselves a notification mail.
        if (organizerEmail == email) {
            continue;
        }

        // Build a nice address for this attendee including the CN.
        QString tname, temail;
        const QString username = KEmailAddress::quoteNameIfNecessary(a->name());
        // ignore the return value from extractEmailAddressAndName() because
        // it will always be false since tusername does not contain "@domain".
        KEmailAddress::extractEmailAddressAndName(username, temail /*byref*/, tname /*byref*/);
        tname += QLatin1String(" <") + email + QLatin1Char('>');

        // Optional Participants and Non-Participants are copied on the email
        if (a->role() == KCalCore::Attendee::OptParticipant
            || a->role() == KCalCore::Attendee::NonParticipant) {
            ccList << tname;
        } else {
            toList << tname;
        }
    }
    if (toList.isEmpty() && ccList.isEmpty()) {
        // Not really to be called a groupware meeting, eh
        qCWarning(PIMKOLAB_LOG) << "There are really no attendees to e-mail";
        return QByteArray();
    }
    QString to;
    if (!toList.isEmpty()) {
        to = toList.join(QLatin1String(", "));
    }
    QString cc;
    if (!ccList.isEmpty()) {
        cc = ccList.join(QLatin1String(", "));
    }

    QString subject;
    if (incidence->type() != KCalCore::Incidence::TypeFreeBusy) {
        KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>();
        subject = inc->summary();
    } else {
        subject = QStringLiteral("Free Busy Object");
    }

    const QString body
        = KCalUtils::IncidenceFormatter::mailBodyStr(incidence);

    return createMessage(/* identity, */ from, to, cc, subject, body, false,
                         bccMe, attachment /*, mailTransport */)->encodedContent();
}

QByteArray mailOrganizer(const KCalCore::IncidenceBase::Ptr &incidence,
//                                 const KPIMIdentities::Identity &identity,
                         const QString &from, bool bccMe, const QString &attachment, const QString &sub /*, const QString &mailTransport*/)
{
    const QString to = incidence->organizer().fullName();
    QString subject = sub;

    if (incidence->type() != KCalCore::Incidence::TypeFreeBusy) {
        KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>();
        if (subject.isEmpty()) {
            subject = inc->summary();
        }
    } else {
        subject = QStringLiteral("Free Busy Message");
    }

    QString body = KCalUtils::IncidenceFormatter::mailBodyStr(incidence);

    return createMessage(/*identity, */ from, to, QString(), subject, body, false,
                         bccMe, attachment /*, mailTransport */)->encodedContent();
}
