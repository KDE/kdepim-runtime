/*
   Copyright (C) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "googlesettingsdialog.h"
#include "googleaccountmanager.h"
#include "googlesettings.h"
#include "googleresource.h"
#include "settings.h"

#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QCheckBox>

#include <KDE/KLocalizedString>
#include <KDE/KComboBox>
#include <KDE/KPushButton>
#include <KDE/KDebug>
#include <KDE/KMessageBox>
#include <KDE/KWindowSystem>
#include <KDE/KIntSpinBox>

#include <LibKGAPI2/Account>
#include <LibKGAPI2/AuthJob>

Q_DECLARE_METATYPE( KGAPI2::Job* )

using namespace KGAPI2;

GoogleSettingsDialog::GoogleSettingsDialog( GoogleAccountManager *accountManager, WId wId, GoogleResource *parent ):
    KDialog(),
    m_parentResource( parent ),
    m_accountManager( accountManager )
{
    KWindowSystem::setMainWindow( this, wId );
    setButtons( Ok | Cancel );

    connect( this, SIGNAL(accepted()),
             this, SLOT(slotSaveSettings()) );

    QWidget *widget = new QWidget( this );
    QVBoxLayout *mainLayout = new QVBoxLayout( widget );
    setMainWidget( widget );

    m_accGroupBox = new QGroupBox( i18n( "Accounts" ), this );
    mainLayout->addWidget( m_accGroupBox );
    QHBoxLayout *accLayout = new QHBoxLayout( m_accGroupBox );

    m_accComboBox = new KComboBox( m_accGroupBox );
    accLayout->addWidget( m_accComboBox, 1 );
    connect( m_accComboBox, SIGNAL(currentIndexChanged(QString)),
             this, SIGNAL(currentAccountChanged(QString)) );

    m_addAccButton = new KPushButton( KIcon( QLatin1String("list-add-user") ), i18n( "&Add" ), m_accGroupBox );
    accLayout->addWidget( m_addAccButton );
    connect( m_addAccButton, SIGNAL(clicked(bool)),
             this, SLOT(slotAddAccountClicked()) );

    m_removeAccButton = new KPushButton( KIcon( QLatin1String("list-remove-user") ), i18n( "&Remove" ), m_accGroupBox );
    accLayout->addWidget( m_removeAccButton );
    connect( m_removeAccButton, SIGNAL(clicked(bool)),
             this, SLOT(slotRemoveAccountClicked()) );

    QGroupBox *refreshBox = new QGroupBox( i18n( "Refresh" ), this );
    mainLayout->addWidget( refreshBox );
    QGridLayout *refreshLayout = new QGridLayout( refreshBox );

    m_enableRefresh = new QCheckBox( i18n( "Enable interval refresh" ), refreshBox );
    m_enableRefresh->setChecked( Settings::self()->enableIntervalCheck() );
    refreshLayout->addWidget( m_enableRefresh, 0, 0, 1, 2 );

    QLabel *label = new QLabel( i18n( "Refresh interval:" ) );
    refreshLayout->addWidget( label, 1, 0 );
    m_refreshSpinBox = new KIntSpinBox( 10, 720, 1, 30, this, 10 );
    m_refreshSpinBox->setSuffix( ki18np( " minute", " minutes" ) );
    m_refreshSpinBox->setEnabled( Settings::self()->enableIntervalCheck() );
    refreshLayout->addWidget( m_refreshSpinBox, 1, 1 );
    connect( m_enableRefresh, SIGNAL(toggled(bool)),
             m_refreshSpinBox, SLOT(setEnabled(bool)) );

    if ( m_enableRefresh->isEnabled() ) {
        m_refreshSpinBox->setValue( Settings::self()->intervalCheckTime() );
    }

    QMetaObject::invokeMethod( this, "reloadAccounts", Qt::QueuedConnection );
}

GoogleSettingsDialog::~GoogleSettingsDialog()
{
}

GoogleAccountManager *GoogleSettingsDialog::accountManager() const
{
    return m_accountManager;
}

KGAPI2::AccountPtr GoogleSettingsDialog::currentAccount() const
{
    return m_accountManager->findAccount( m_accComboBox->currentText() );
}

void GoogleSettingsDialog::reloadAccounts()
{
    disconnect( m_accComboBox, SIGNAL(currentIndexChanged(QString)),
                this, SIGNAL(currentAccountChanged(QString)) );

    m_accComboBox->clear();

    const AccountsList accounts = m_accountManager->listAccounts();
    Q_FOREACH( const AccountPtr &account, accounts ) {
        m_accComboBox->addItem( account->accountName() );
    }

    int index = m_accComboBox->findText( m_parentResource->settings()->account(), Qt::MatchExactly );
    if ( index > -1 ) {
        m_accComboBox->setCurrentIndex( index );
    }

    disconnect( m_accComboBox, SIGNAL(currentIndexChanged(QString)),
                this, SIGNAL(currentAccountChanged(QString)) );

    Q_EMIT currentAccountChanged( m_accComboBox->currentText() );
}

void GoogleSettingsDialog::slotAddAccountClicked()
{
    AccountPtr account( new Account() );
    // FIXME: We need a proper API for this
    account->addScope( Account::contactsScopeUrl() );
    account->addScope( Account::calendarScopeUrl() );
    account->addScope( Account::tasksScopeUrl() );
    account->addScope( Account::accountInfoEmailScopeUrl() );
    account->addScope( Account::accountInfoScopeUrl() );

    AuthJob *authJob = new AuthJob( account,
                                    m_parentResource->settings()->clientId(),
                                    m_parentResource->settings()->clientSecret() );
    connect( authJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotAccountAuthenticated(KGAPI2::Job*)) );
}

void GoogleSettingsDialog::slotRemoveAccountClicked()
{
    const AccountPtr account = currentAccount();
    if ( !account ) {
        return;
    }

    if ( KMessageBox::warningYesNo(
                this,
                i18n( "Do you really want to revoke access to account <b>%1</b>?"
                      "<p>This will revoke access to all resources using this account!</p>",
                      account->accountName() ),
                i18n( "Revoke Access?" ),
                KStandardGuiItem::yes(),
                KStandardGuiItem::no(),
                QString(),
                KMessageBox::Dangerous ) != KMessageBox::Yes ) {

        return;
    }

    m_accountManager->removeAccount( account->accountName() );
    reloadAccounts();
}

void GoogleSettingsDialog::slotAccountAuthenticated( Job *job )
{
    AuthJob *authJob = qobject_cast<AuthJob*>(job);
    const AccountPtr account = authJob->account();

    if ( !m_accountManager->storeAccount( account ) ) {
        kWarning() << "Failed to add account to KWallet";
    }

    reloadAccounts();
}

bool GoogleSettingsDialog::handleError( Job *job )
{
    if (( job->error() == KGAPI2::NoError ) || ( job->error() == KGAPI2::OK )) {
        return true;
    }

    if ( job->error() == KGAPI2::Unauthorized ) {
        kDebug() << job << job->errorString();
        const AccountPtr account = currentAccount();
        const QList<QUrl> resourceScopes = m_parentResource->scopes();
        Q_FOREACH(const QUrl &scope, resourceScopes) {
            if ( !account->scopes().contains( scope ) ) {
                account->addScope( scope );
            }
        }

        AuthJob *authJob = new AuthJob( account, m_parentResource->settings()->clientId(), 
                                        m_parentResource->settings()->clientSecret(), this );
        authJob->setProperty( JOB_PROPERTY, QVariant::fromValue( job ) );
        connect( authJob, SIGNAL(finished(KGAPI2::Job*)),
                 this, SLOT(slotAuthJobFinished(KGAPI2::Job*)) );

        return false;
    }

    KMessageBox::sorry( this, job->errorString() );
    job->deleteLater();
    return false;
}

void GoogleSettingsDialog::slotAuthJobFinished( Job *job )
{
    kDebug();

    if ( job->error() != KGAPI2::NoError ) {
        KMessageBox::sorry( this, job->errorString() );
        return;
    }

    AuthJob *authJob = qobject_cast<AuthJob*>( job );
    const AccountPtr account = authJob->account();
    if ( !m_accountManager->storeAccount( account ) ) {
        kWarning() << "Failed to store account in KWallet";
    }

    KGAPI2::Job *otherJob = job->property( JOB_PROPERTY ).value<KGAPI2::Job *>();
    otherJob->setAccount( account );
    otherJob->restart();

    job->deleteLater();
}

void GoogleSettingsDialog::slotSaveSettings()
{
    Settings::self()->setEnableIntervalCheck( m_enableRefresh->isChecked() );
    Settings::self()->setIntervalCheckTime( m_refreshSpinBox->value() );

    saveSettings();
}
