/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "gmailconfigdialog.h"
#include "gmailsettings.h"
#include "gmailresource.h"
#include "ui_gmailconfigdialog.h"
#include <folderarchivesettingpage.h>

#include <mailtransport/transport.h>

#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>

#include <akonadi/resourcesettings.h>

#include <libkgapi2/account.h>
#include <libkgapi2/authjob.h>

#include <imap/imapaccount.h>
#include <imap/subscriptiondialog.h>

#include <KLocalizedString>
#include <KMessageBox>

#include <QPointer>

GmailConfigDialog::GmailConfigDialog(GmailResource *resource, WId parent)
    : KDialog()
    , m_parentResource(resource)
    , m_ui(new Ui::GmailConfigDialog)
    , m_subscriptionsChanged(false)
    , m_shouldClearCache(false)
{
    m_parentResource->settings()->setWinId(parent);

    m_ui->setupUi(mainWidget());
    m_folderArchiveSettingPage = new FolderArchiveSettingPage(resource->identifier());
    m_ui->tabWidget->addTab(m_folderArchiveSettingPage, i18n("Folder Archive"));

    m_ui->checkInterval->setSuffix( ki18np( " minute", " minutes" ) );
    m_ui->checkInterval->setRange( Akonadi::ResourceSettings::self()->minimumCheckInterval(), 10000, 1 );


    m_identityManager = new KPIMIdentities::IdentityManager( false, this, "mIdentityManager" );
    m_identityCombobox = new KPIMIdentities::IdentityCombo( m_identityManager, this );
    m_ui->identityLabel->setBuddy( m_identityCombobox );
    m_ui->identityLayout->addWidget( m_identityCombobox, 1 );
    m_ui->identityLabel->setBuddy( m_identityCombobox );

    connect(m_ui->subscriptionEnabled, SIGNAL(toggled(bool)),
            this, SLOT(slotSubcriptionCheckboxChanged()) );
    connect(m_ui->subscriptionButton, SIGNAL(clicked(bool)),
            this, SLOT(slotManageSubscriptions()));

    connect(m_ui->authenticateButton, SIGNAL(clicked(bool)),
            this, SLOT(slotAuthenticate()));
    connect(m_ui->changeAuthButton, SIGNAL(clicked(bool)),
            this, SLOT(slotAuthenticate()));

    connect(m_ui->useDefaultIdentityCheck, SIGNAL(toggled(bool)),
             this, SLOT(slotIdentityCheckboxChanged()));
    connect(m_ui->enableMailCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotMailCheckboxChanged()));

    connect(m_parentResource->settings(), SIGNAL(accountRequestCompleted(KGAPI2::AccountPtr,bool)),
            this, SLOT(onAccountRequestCompleted(KGAPI2::AccountPtr,bool)));

    readSettings();
    slotComplete();
    slotSubcriptionCheckboxChanged();
    slotIdentityCheckboxChanged();

    //connect(this, SIGNAL(applyClicked()),
            //this, SLOT(applySettings()) );
    connect(this, SIGNAL(okClicked()),
            this, SLOT(applySettings()) );
}

GmailConfigDialog::~GmailConfigDialog()
{
    delete m_ui;
}

bool GmailConfigDialog::shouldClearCache() const
{
    return m_shouldClearCache;
}

void GmailConfigDialog::slotSubcriptionCheckboxChanged()
{
    m_ui->subscriptionButton->setEnabled(m_ui->subscriptionEnabled->isChecked());
}

void GmailConfigDialog::slotIdentityCheckboxChanged()
{
    m_identityCombobox->setEnabled(!m_ui->useDefaultIdentityCheck->isChecked());
}

void GmailConfigDialog::slotMailCheckboxChanged()
{
    m_ui->checkInterval->setEnabled(m_ui->enableMailCheckBox->isChecked());
}

void GmailConfigDialog::applySettings()
{
    m_folderArchiveSettingPage->writeSettings();
    m_parentResource->setName(m_ui->usernameLabel->text());

    GmailSettings *settings = static_cast<GmailSettings*>(m_parentResource->settings());
    settings->setImapServer(QLatin1String("imap.gmail.com"));
    settings->setImapPort(993);
    settings->setUserName(m_account->accountName());
    settings->setPassword(m_account->accessToken());
    settings->setRefreshToken(m_account->refreshToken());
    settings->setSafety(QLatin1String("SSL"));
    settings->setSubscriptionEnabled(m_ui->subscriptionEnabled->isChecked());
    settings->setIntervalCheckTime(m_ui->checkInterval->value());
    settings->setDisconnectedModeEnabled(m_ui->disconnectedModeEnabled->isChecked());

    /* Gmail does not support sieve */
    settings->setSieveSupport(false);

    settings->setAutomaticExpungeEnabled(m_ui->autoExpungeCheck->isChecked());
    settings->setUseDefaultIdentity(m_ui->useDefaultIdentityCheck->isChecked());
    if (!m_ui->useDefaultIdentityCheck->isChecked()) {
        settings->setAccountIdentity(m_identityCombobox->currentIdentity());
    }

    settings->setIntervalCheckEnabled(m_ui->enableMailCheckBox->isChecked());
    if (m_ui->enableMailCheckBox->isChecked()) {
        settings->setIntervalCheckTime( m_ui->checkInterval->value() );
    }

    settings->writeConfig();

    if (m_oldResourceName != m_account->accountName() && !m_account->accountName().isEmpty()) {
        settings->renameRootCollection(m_account->accountName());
    }
}

void GmailConfigDialog::readSettings()
{
    m_folderArchiveSettingPage->loadSettings();
    m_ui->usernameLabel->setText(m_parentResource->name());
    m_oldResourceName = m_parentResource->name();

    GmailSettings *settings = static_cast<GmailSettings*>(m_parentResource->settings());

    m_account = KGAPI2::AccountPtr(new KGAPI2::Account);
    if (!m_parentResource->name().startsWith(m_parentResource->defaultName())) {
        m_account->setAccountName(m_parentResource->name());
        m_ui->usernameLabel->setText(m_account->accountName());
        m_ui->authenticateButton->setVisible(false);
        m_ui->currentAccountBox->setVisible(true);

        bool rejected = false;
        const QString accessToken = settings->password(&rejected);
        const QString refreshToken = settings->refreshToken(&rejected);
        if ( rejected ) {
            //m_ui->password->setEnabled( false );
            KMessageBox::information(0, i18n("Could not access KWallet. If you want to use Gmail resource, you have to activate it."));
        } else {
            m_account->setAccessToken(accessToken);
            m_account->setRefreshToken(refreshToken);
        }
    } else {
        m_ui->currentAccountBox->setVisible(false);
        m_ui->authenticateButton->setVisible(true);
    }

    m_ui->subscriptionEnabled->setChecked(settings->subscriptionEnabled());

    m_ui->enableMailCheckBox->setChecked(settings->intervalCheckEnabled());
    m_ui->checkInterval->setEnabled(m_ui->enableMailCheckBox->isChecked());
    m_ui->checkInterval->setValue(settings->intervalCheckTime());
    m_ui->disconnectedModeEnabled->setChecked(settings->disconnectedModeEnabled());

    m_ui->useDefaultIdentityCheck->setChecked(settings->useDefaultIdentity());
    if (!m_ui->useDefaultIdentityCheck->isChecked())
        m_identityCombobox->setCurrentIdentity(settings->accountIdentity());


    m_ui->autoExpungeCheck->setChecked(settings->automaticExpungeEnabled());
}

void GmailConfigDialog::slotComplete()
{
    const bool ok = (m_account || !m_account->accountName().isEmpty());
    button(KDialog::Ok)->setEnabled(ok);
}

void GmailConfigDialog::slotManageSubscriptions()
{
    ImapAccount account;

    account.setServer(QLatin1String("imap.gmail.com"));
    account.setPort(993);
    account.setUserName(m_account->accountName());
    account.setSubscriptionEnabled(m_ui->subscriptionEnabled->isChecked());

    account.setEncryptionMode(KIMAP::LoginJob::SslV3);
    account.setAuthenticationMode(KIMAP::LoginJob::XOAuth2);

    QPointer<SubscriptionDialog> subscriptions = new SubscriptionDialog( this );
    subscriptions->setCaption(i18n("Serverside Subscription"));
    subscriptions->setWindowIcon(KIcon(QLatin1String("network-server")));
    subscriptions->connectAccount(account, m_account->accessToken());
    m_subscriptionsChanged = subscriptions->isSubscriptionChanged();

    subscriptions->exec();
    delete subscriptions;

    m_ui->subscriptionEnabled->setChecked(account.isSubscriptionEnabled());
}

void GmailConfigDialog::slotAuthenticate()
{
    GmailSettings *settings = static_cast<GmailSettings*>(m_parentResource->settings());
    settings->clearCachedPassword();
    settings->storeAccount(KGAPI2::AccountPtr());
    settings->requestAccount(true);
    m_shouldClearCache = true;
}

void GmailConfigDialog::onAccountRequestCompleted(const KGAPI2::AccountPtr &account, bool userRejected)
{
    if (userRejected || account.isNull()) {
        m_account = KGAPI2::AccountPtr();

        m_ui->currentAccountBox->setVisible(false);
        m_ui->authenticateButton->setVisible(true);
    } else {
        m_account = account;
        m_ui->currentAccountBox->setVisible(true);
        m_ui->usernameLabel->setText(m_account->accountName());
        m_ui->authenticateButton->setVisible(false);
    }

    GmailSettings *settings = static_cast<GmailSettings*>(m_parentResource->settings());
    settings->storeAccount(m_account);
    slotComplete();
}

