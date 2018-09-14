/*
   Copyright (C) 2013-2018 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "specialnotifierjob.h"
#include "newmailnotifiershowmessagejob.h"
#include "newmailnotifieragentsettings.h"

#include <Akonadi/Contact/ContactSearchJob>
#include <ItemFetchJob>
#include <ItemFetchScope>
#include <AkonadiCore/ItemDeleteJob>
#include <Akonadi/KMime/MessageParts>

#include <KNotification>
#include <KEmailAddress>

#include <KMime/Message>

#include <KLocalizedString>
#include "newmailnotifier_debug.h"

SpecialNotifierJob::SpecialNotifierJob(const QStringList &listEmails, const QString &path, Akonadi::Item::Id id, QObject *parent)
    : QObject(parent)
    , mListEmails(listEmails)
    , mPath(path)
    , mItemId(id)
{
    Akonadi::Item item(mItemId);
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item, this);
    job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope, true);

    connect(job, &Akonadi::ItemFetchJob::result, this, &SpecialNotifierJob::slotItemFetchJobDone);
}

SpecialNotifierJob::~SpecialNotifierJob()
{
}

void SpecialNotifierJob::setDefaultPixmap(const QPixmap &pixmap)
{
    mDefaultPixmap = pixmap;
}

void SpecialNotifierJob::slotItemFetchJobDone(KJob *job)
{
    if (job->error()) {
        qCWarning(NEWMAILNOTIFIER_LOG) << job->errorString();
        deleteLater();
        return;
    }

    const Akonadi::Item::List lst = qobject_cast<Akonadi::ItemFetchJob *>(job)->items();
    if (lst.count() == 1) {
        mItem = lst.first();
        if (!mItem.hasPayload<KMime::Message::Ptr>()) {
            qCDebug(NEWMAILNOTIFIER_LOG) << " message has not payload.";
            deleteLater();
            return;
        }

        const KMime::Message::Ptr mb = mItem.payload<KMime::Message::Ptr>();
        mFrom = mb->from()->asUnicodeString();
        mSubject = mb->subject()->asUnicodeString();
        if (NewMailNotifierAgentSettings::showPhoto()) {
            Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob(this);
            job->setLimit(1);
            job->setQuery(Akonadi::ContactSearchJob::Email, KEmailAddress::firstEmailAddress(mFrom).toLower(), Akonadi::ContactSearchJob::ExactMatch);
            connect(job, &Akonadi::ItemFetchJob::result, this, &SpecialNotifierJob::slotSearchJobFinished);
        } else {
            emitNotification(mDefaultPixmap);
            deleteLater();
        }
    } else {
        qCWarning(NEWMAILNOTIFIER_LOG) << " Found item different from 1: " << lst.count();
        deleteLater();
        return;
    }
}

void SpecialNotifierJob::slotSearchJobFinished(KJob *job)
{
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob *>(job);
    if (searchJob->error()) {
        qCWarning(NEWMAILNOTIFIER_LOG) << "Unable to fetch contact:" << searchJob->errorText();
        emitNotification(mDefaultPixmap);
        return;
    }
    if (!searchJob->contacts().isEmpty()) {
        const KContacts::Addressee addressee = searchJob->contacts().at(0);
        const KContacts::Picture photo = addressee.photo();
        const QImage image = photo.data();
        if (image.isNull()) {
            emitNotification(mDefaultPixmap);
        } else {
            emitNotification(QPixmap::fromImage(image));
        }
    } else {
        emitNotification(mDefaultPixmap);
    }
}

void SpecialNotifierJob::emitNotification(const QPixmap &pixmap)
{
    if (NewMailNotifierAgentSettings::excludeEmailsFromMe()) {
        for (const QString &email : qAsConst(mListEmails)) {
            if (mFrom.contains(email)) {
                //Exclude this notification
                deleteLater();
                return;
            }
        }
    }

    QStringList result;
    if (NewMailNotifierAgentSettings::showFrom()) {
        result << i18n("From: %1", mFrom.toHtmlEscaped());
    }
    if (NewMailNotifierAgentSettings::showSubject()) {
        QString subject = mSubject.simplified();
        if (subject.length() > 80) {
            subject.truncate(80);
            subject += QStringLiteral("...");
        }
        result << i18n("Subject: %1", subject.toHtmlEscaped());
    }
    if (NewMailNotifierAgentSettings::showFolder()) {
        result << i18n("In: %1", mPath);
    }

    if (NewMailNotifierAgentSettings::textToSpeakEnabled()) {
        if (!NewMailNotifierAgentSettings::textToSpeak().isEmpty()) {
            QString message = NewMailNotifierAgentSettings::textToSpeak();
            message.replace(QLatin1String("%s"), mSubject.toHtmlEscaped());
            message.replace(QLatin1String("%f"), mFrom.toHtmlEscaped());
            Q_EMIT say(message);
        }
    }

    if (NewMailNotifierAgentSettings::showButtonToDisplayMail()) {
        KNotification *notification = new KNotification(QStringLiteral("new-email"), nullptr,
                                                        NewMailNotifierAgentSettings::keepPersistentNotification() ? KNotification::Persistent
                                                        | KNotification::SkipGrouping : KNotification::CloseOnTimeout);
        notification->setText(result.join(QLatin1Char('\n')));
        notification->setPixmap(pixmap);
        notification->setActions(QStringList() << i18n("Show mail...") << i18n("Mark As Read") << i18n("Delete"));

        connect(notification, QOverload<unsigned int>::of(&KNotification::activated), this, &SpecialNotifierJob::slotActivateNotificationAction);
        connect(notification, &KNotification::closed, this, &SpecialNotifierJob::deleteLater);

        notification->sendEvent();
    } else {
        Q_EMIT displayNotification(pixmap, result.join(QLatin1Char('\n')));
        deleteLater();
    }
}

void SpecialNotifierJob::slotActivateNotificationAction(unsigned int index)
{
    //Index == 0 => is the default action. We don't have it.
    switch (index) {
    case 1:
        slotOpenMail();
        return;
    case 2:
        slotMarkAsRead();
        return;
    case 3:
        slotDeleteMessage();
        return;
    }
    qCWarning(NEWMAILNOTIFIER_LOG) << " SpecialNotifierJob::slotActivateNotificationAction unknown index " << index;
}

void SpecialNotifierJob::slotDeleteMessage()
{
    Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob(mItem);
    connect(job, &Akonadi::ItemDeleteJob::result, this, &SpecialNotifierJob::deleteItemDone);
}

void SpecialNotifierJob::deleteItemDone(KJob *job)
{
    if (job->error()) {
        qCWarning(NEWMAILNOTIFIER_LOG) << "SpecialNotifierJob::deleteItemDone error:" << job->errorString();
    }
}

void SpecialNotifierJob::slotMarkAsRead()
{
    Akonadi::MessageStatus messageStatus;
    messageStatus.setRead(true);
    Akonadi::MarkAsCommand *markAsReadAllJob = new Akonadi::MarkAsCommand(messageStatus, Akonadi::Item::List() << mItem);
    connect(markAsReadAllJob, &Akonadi::MarkAsCommand::result, this, &SpecialNotifierJob::slotMarkAsResult);
    markAsReadAllJob->execute();

}

void SpecialNotifierJob::slotMarkAsResult(Akonadi::MarkAsCommand::Result result)
{
    switch (result) {
    case Akonadi::MarkAsCommand::Undefined:
        qCDebug(NEWMAILNOTIFIER_LOG) << "SpecialNotifierJob undefined result";
        break;
    case Akonadi::MarkAsCommand::OK:
        qCDebug(NEWMAILNOTIFIER_LOG) << "SpecialNotifierJob Done";
        break;
    case Akonadi::MarkAsCommand::Canceled:
        qCDebug(NEWMAILNOTIFIER_LOG) << "SpecialNotifierJob was canceled";
        break;
    case Akonadi::MarkAsCommand::Failed:
        qCDebug(NEWMAILNOTIFIER_LOG) << "SpecialNotifierJob was failed";
        break;
    }
}

void SpecialNotifierJob::slotOpenMail()
{
    NewMailNotifierShowMessageJob *job = new NewMailNotifierShowMessageJob(mItem.id());
    job->start();
}
