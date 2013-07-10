/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#include <KLocale>

#include <QTextDocument>

SpecialNotifierJob::SpecialNotifierJob(const QString &path, Akonadi::Item::Id id, QObject *parent)
    : QObject(parent),
      mPath(path)
{
    Akonadi::Item item(id);
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
        kdWarning()<<" Found item different from 1: "<<lst.count();
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
        deleteLater();
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
    deleteLater();
}

void SpecialNotifierJob::emitNotification(const QPixmap &pixmap)
{
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

    emit displayNotification(pixmap, result.join(QLatin1String("\n")));
}

#include "specialnotifierjob.moc"
