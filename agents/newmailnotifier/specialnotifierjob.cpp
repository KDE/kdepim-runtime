/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "newmailnotifiershowmessagejob.h"
#include "specialnotifierjob.h"
#include "util.h"
#include "newmailnotifieragentsettings.h"

#include <Akonadi/Contact/ContactSearchJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <akonadi/kmime/messageparts.h>

#include <KNotification>
#include <KPIMUtils/Email>

#include <KMime/Message>

#include <KLocalizedString>

#include <QTextDocument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>

SpecialNotifierJob::SpecialNotifierJob(const QStringList &listEmails, const QString &path, Akonadi::Item::Id id, QObject *parent)
    : QObject(parent),
      mListEmails(listEmails),
      mPath(path),
      mItemId(id)
{
    Akonadi::Item item(mItemId);
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item, this );
    job->fetchScope().fetchPayloadPart( Akonadi::MessagePart::Envelope, true );

    connect( job, SIGNAL(result(KJob*)), SLOT(slotItemFetchJobDone(KJob*)) );
}

SpecialNotifierJob::~SpecialNotifierJob()
{

}

void SpecialNotifierJob::slotItemFetchJobDone(KJob *job)
{
    if ( job->error() ) {
        kWarning() << job->errorString();
        deleteLater();
        return;
    }

    const Akonadi::Item::List lst = qobject_cast<Akonadi::ItemFetchJob*>( job )->items();
    if (lst.count() == 1) {
        const Akonadi::Item item = lst.first();
        if ( !item.hasPayload<KMime::Message::Ptr>() ) {
            kDebug()<<" message has not payload.";
            deleteLater();
            return;
        }

        const KMime::Message::Ptr mb = item.payload<KMime::Message::Ptr>();
        mFrom = mb->from()->asUnicodeString();
        mSubject = mb->subject()->asUnicodeString();
        if (NewMailNotifierAgentSettings::showPhoto()) {
            Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob( this );
            job->setLimit( 1 );
            job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::firstEmailAddress(mFrom).toLower(), Akonadi::ContactSearchJob::ExactMatch );
            connect( job, SIGNAL(result(KJob*)), SLOT(slotSearchJobFinished(KJob*)) );
        } else {
            emitNotification(Util::defaultPixmap());
            deleteLater();
        }
    } else {
        kWarning()<<" Found item different from 1: "<<lst.count();
        deleteLater();
        return;
    }
}

void SpecialNotifierJob::slotSearchJobFinished( KJob *job )
{
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
    if ( searchJob->error() ) {
        kWarning() << "Unable to fetch contact:" << searchJob->errorText();
        emitNotification(Util::defaultPixmap());
        return;
    }
    if (!searchJob->contacts().isEmpty()) {
        const KABC::Addressee addressee = searchJob->contacts().first();
        const KABC::Picture photo = addressee.photo();
        const QImage image = photo.data();
        if (image.isNull()) {
            emitNotification(Util::defaultPixmap());
        } else {
            emitNotification(QPixmap::fromImage(image));
        }
    } else {
        emitNotification(Util::defaultPixmap());
    }
}

void SpecialNotifierJob::emitNotification(const QPixmap &pixmap)
{
    if (NewMailNotifierAgentSettings::excludeEmailsFromMe()) {
        Q_FOREACH( const QString &email, mListEmails) {
            if (mFrom.contains(email)) {
                //Exclude this notification
                deleteLater();
                return;
            }
        }
    }

    QStringList result;
    if (NewMailNotifierAgentSettings::showFrom()) {
        result << i18n("From: %1", Qt::escape(mFrom));
    }
    if (NewMailNotifierAgentSettings::showSubject()) {
        QString subject(mSubject);
        if (subject.length()> 80) {
            subject.truncate(80);
            subject += QLatin1String("...");
        }
        result << i18n("Subject: %1", Qt::escape(subject));
    }
    if (NewMailNotifierAgentSettings::showFolder()) {
        result << i18n("In: %1", mPath);
    }

    if (NewMailNotifierAgentSettings::textToSpeakEnabled()) {
        if (!NewMailNotifierAgentSettings::textToSpeak().isEmpty()) {
            if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QLatin1String("org.kde.kttsd"))) {
                QDBusInterface ktts(QLatin1String("org.kde.kttsd"), QLatin1String("/KSpeech"), QLatin1String("org.kde.KSpeech"));
                QString message = NewMailNotifierAgentSettings::textToSpeak();
                message.replace(QLatin1String("%s"), Qt::escape(mSubject));
                message.replace(QLatin1String("%f"), Qt::escape(mFrom));
                ktts.asyncCall(QLatin1String("say"), message, 0);
            }
        }
    }

    if (NewMailNotifierAgentSettings::showButtonToDisplayMail()) {
        KNotification *notification= new KNotification ( QLatin1String("new-email"), 0, KNotification::CloseOnTimeout);
        notification->setText( result.join(QLatin1String("\n")) );
        notification->setPixmap( pixmap );
        notification->setActions( QStringList() << i18n( "Show mail..." ) );

        connect(notification, SIGNAL(activated(uint)), this, SLOT(slotOpenMail()) );
        connect(notification, SIGNAL(closed()), this, SLOT(deleteLater()));

        notification->sendEvent();
        if ( NewMailNotifierAgentSettings::beepOnNewMails() ) {
            KNotification::beep();
        }
    } else {
        emit displayNotification(pixmap, result.join(QLatin1String("\n")));
        deleteLater();
    }
}

void SpecialNotifierJob::slotOpenMail()
{
    NewMailNotifierShowMessageJob *job = new NewMailNotifierShowMessageJob(mItemId);
    job->start();
}
