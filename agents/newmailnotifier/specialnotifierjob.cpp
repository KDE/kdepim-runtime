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

#include <Akonadi/Contact/ContactSearchJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <akonadi/kmime/messageparts.h>

#include <KNotification>
#include <KPIMUtils/Email>

#include <KMime/Message>

#include <KLocale>

SpecialNotifierJob::SpecialNotifierJob(Akonadi::Item::Id id, QObject *parent)
    : QObject(parent)
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

    Akonadi::Item::List lst = qobject_cast<Akonadi::ItemFetchJob*>( job )->items();
    if (lst.count() == 1) {
        Akonadi::Item item = lst.first();
        if ( !item.hasPayload<KMime::Message::Ptr>() ) {
          deleteLater();
          return;
        }
        const KMime::Message::Ptr mb = item.payload<KMime::Message::Ptr>();

        mFrom = mb->from()->asUnicodeString();
        mSubject = mb->subject()->asUnicodeString();
        Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob( this );
        job->setLimit( 1 );
        job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::firstEmailAddress(mFrom).toLower(), Akonadi::ContactSearchJob::ExactMatch );
        connect( job, SIGNAL(result(KJob*)), SLOT(slotSearchJobFinished(KJob*)) );
    } else {
        deleteLater();
        return;
    }
}

void SpecialNotifierJob::slotSearchJobFinished( KJob *job )
{
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
    if ( searchJob->error() ) {
        kWarning() << "Unable to fetch contact:" << searchJob->errorText();
        Util::showNotification(Util::defaultPixmap(), i18n("from: %1 \nSubject: %2",mFrom, mSubject));
        deleteLater();
        return;
    }
    if (!searchJob->contacts().isEmpty()) {
        const KABC::Addressee addressee = searchJob->contacts().first();
        const KABC::Picture photo = addressee.photo();
        const QImage image = photo.data();
        Util::showNotification(QPixmap::fromImage(image), i18n("from: %1 \nSubject: %2",mFrom, mSubject));
    } else {
        Util::showNotification(Util::defaultPixmap(), i18n("from: %1 \nSubject: %2",mFrom, mSubject));
    }
    deleteLater();
}


#include "specialnotifierjob.moc"
