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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "googleresource.h"
#include "settings.h"

#include "getcredentialsjob.h"

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/CollectionFetchScope>

#include <KDE/KDialog>
#include <KDE/KLocale>

#include <LibKGAPI2/Account>
#include <LibKGAPI2/AccountInfo/AccountInfo>
#include <LibKGAPI2/AccountInfo/AccountInfoFetchJob>
#include <QtCore/QTimer>

#define ACCESS_TOKEN_PROPERTY "_AccessToken"

Q_DECLARE_METATYPE( KGAPI2::Job* )

using namespace KGAPI2;
using namespace Akonadi;

GoogleResource::GoogleResource( const QString &id ):
    ResourceBase( id ),
    AgentBase::ObserverV2()
{
    KGlobal::locale()->insertCatalog( "akonadi_google_resource" );
    connect( this, SIGNAL(abortRequested()),
            this, SLOT(slotAbortRequested()) );
    connect( this, SIGNAL(reloadConfiguration()),
            this, SLOT(reloadConfig()) );

    setNeedsNetwork( true );
    setOnline( true );

    changeRecorder()->itemFetchScope().fetchFullPayload( true );
    changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
    changeRecorder()->fetchCollection( true );
    changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );

    // updateResourceName() calls pure-virtual method
    QTimer::singleShot( 0, this, SLOT(updateResourceName()) );
}

GoogleResource::~GoogleResource()
{
}

AccountPtr GoogleResource::account() const
{
    return m_account;
}

void GoogleResource::aboutToQuit()
{
    slotAbortRequested();
}

void GoogleResource::abort()
{
    cancelTask( i18n( "Aborted" ) );
}

void GoogleResource::slotAbortRequested()
{
    abort();
}

void GoogleResource::reloadConfig()
{
    kDebug() << "Configuration changed";

    if ( !settings()->accountId() ) {
        emit status( NotConfigured, i18n( "Account is not configured" ) );
        return;
    }

    GetCredentialsJob *cj = new GetCredentialsJob( settings()->accountId(), this );
    connect( cj, SIGNAL(finished(KJob*)),
             this, SLOT(slotTokensReceived(KJob*)) );
    cj->start();
}

void GoogleResource::slotTokensReceived( KJob* job )
{
    GetCredentialsJob *cj = qobject_cast<GetCredentialsJob*>( job );
    if ( cj->error() ) {
        kDebug() << "Error: " << cj->errorString();
        cancelTask( i18n(" Failed to refresh tokens" ) );
        return;
    }

    const QVariantMap data = cj->credentialsData();
    const QString accessToken = data.value( QLatin1String( "AccessToken" ) ).toString();

    KGAPI2::Job *otherJob = 0;
    if ( !job->property( JOB_PROPERTY ).isNull() ) {
        otherJob = job->property( JOB_PROPERTY ).value<KGAPI2::Job*>();
    }

    if  ( settings()->accountName().isEmpty() ) {
        // This is just a temporary account without proper name. We will use it
        // to fetch real account name
        AccountPtr account(new Account( i18n( "Unknown account" ), accessToken ) );
        AccountInfoFetchJob *aiJob = new AccountInfoFetchJob( account, this );
        aiJob->setProperty( ACCESS_TOKEN_PROPERTY, accessToken );
        if ( otherJob ) {
            aiJob->setProperty( JOB_PROPERTY, QVariant::fromValue( otherJob ) );
        }
        connect( aiJob, SIGNAL(finished(KGAPI2::Job*)),
                this, SLOT(slotAccountInfoReceived(KGAPI2::Job*)) );
    } else {
        m_account = AccountPtr( new Account( settings()->accountName(),
                                             accessToken ) );
        finishAuthentication( otherJob );
    }
}

void GoogleResource::slotAccountInfoReceived( KGAPI2::Job* job )
{
    if ( !handleError( job ) ) {
        emit error( job->errorString() );
        cancelTask( i18n( "Failed to refresh tokens") );
        return;
    }

    AccountInfoFetchJob *aiJob = qobject_cast<AccountInfoFetchJob*>( job );
    Q_ASSERT( aiJob );
    aiJob->deleteLater();

    const AccountPtr account = job->account();

    if ( aiJob->items().count() != 1 ) {
        kWarning() << "AccountInfoFetchJob returned unexpected amount of results";
        emit error( i18n( "Invalid reply" ) );
        cancelTask( i18n( "Failed to refresh tokens") );
        return;
    }

    AccountInfoPtr info = aiJob->items().first().dynamicCast<AccountInfo>();
    settings()->setAccountName( info->email() );
    m_account = AccountPtr( new Account( info->email(),
                                         aiJob->property( ACCESS_TOKEN_PROPERTY ).toString() ) );


    settings()->writeConfig();

    KGAPI2::Job *otherJob = 0;
    if ( job->property( JOB_PROPERTY ).isNull() ) {
        otherJob = job->property( JOB_PROPERTY ).value<KGAPI2::Job*>();
    }

    finishAuthentication( otherJob, true );
}

void GoogleResource::finishAuthentication( KGAPI2::Job* job, bool forceConfig )
{
    updateResourceName();
    emit status( Idle, i18nc( "@info:status", "Ready" ) );

    if ( job ) {
        job->setAccount( m_account );
        job->restart();
    } else {
        if ( forceConfig ) {
            configure( 0 ); /* FIXME: Get correct wId? How? */
        } else {
            synchronize();
        }
    }
}

bool GoogleResource::handleError( KGAPI2::Job *job )
{
    if (( job->error() == KGAPI2::NoError ) || ( job->error() == KGAPI2::OK )) {
        return true;
    }

    if ( job->error() == KGAPI2::Unauthorized ) {
        kDebug() << job << job->errorString();
        GetCredentialsJob *cj = new GetCredentialsJob( settings()->accountId(), this );
        cj->setProperty( JOB_PROPERTY, QVariant::fromValue( job ) );
        connect( cj, SIGNAL(finished(KJob*)),
                 this, SLOT(slotTokensReceived(KJob*)) );
        return false;
    }

    cancelTask( job->errorString() );
    job->deleteLater();
    return false;
}

bool GoogleResource::canPerformTask()
{
    if ( !m_account ) {
        cancelTask( i18nc( "@info:status", "Resource is not configured" ) );
        emit status( NotConfigured, i18nc( "@info:status", "Resource is not configured" ) );
        return false;
    }

    return true;
}

void GoogleResource::slotGenericJobFinished( KGAPI2::Job *job )
{
    if ( !handleError( job ) ) {
        return;
    }

    const Item item = job->property( ITEM_PROPERTY ).value<Item>();
    const Collection collection = job->property( COLLECTION_PROPERTY ).value<Collection>();
    if ( item.isValid() ) {
        changeCommitted( item );
    } else if ( collection.isValid() ) {
        changeCommitted( collection );
    } else {
        taskDone();
    }

    emit status( Idle, i18nc( "@info:status", "Ready" ) );

    job->deleteLater();
}

void GoogleResource::emitPercent( KGAPI2::Job *job, int processedItems, int totalItems )
{
    Q_UNUSED( job );

    emit percent( ( ( float ) processedItems / ( float ) totalItems ) * 100 );
}

bool GoogleResource::retrieveItem( const Item &item, const QSet< QByteArray > &parts )
{
    Q_UNUSED( parts );

    /* We don't support fetching parts, the item is already fully stored. */
    itemRetrieved( item );

    return true;
}


#include "googleresource.moc"
