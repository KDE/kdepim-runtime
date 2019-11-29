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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "googlesettingsdialog.h"
#include "googleaccountmanager.h"
#include "googlesettings.h"
#include "googleresource.h"
#include "settings.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>

#include <KLocalizedString>
#include <QComboBox>
#include <QPushButton>
#include <QDebug>
#include <KMessageBox>
#include <KWindowSystem>
#include <KPluralHandlingSpinBox>

#include <KGAPI/AuthJob>
#include <QDialogButtonBox>
#include <QVBoxLayout>

Q_DECLARE_METATYPE(KGAPI2::Job *)

using namespace KGAPI2;

GoogleSettingsDialog::GoogleSettingsDialog(GoogleAccountManager *accountManager, WId wId, GoogleResource *parent)
    : QDialog()
    , m_parentResource(parent)
    , m_accountManager(accountManager)
{
    setAttribute(Qt::WA_NativeWindow, true);
    KWindowSystem::setMainWindow(windowHandle(), wId);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GoogleSettingsDialog::slotSaveSettings);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &GoogleSettingsDialog::reject);

    QWidget *widget = new QWidget(this);
    topLayout->addWidget(widget);
    topLayout->addWidget(buttonBox);
    QVBoxLayout *mainLayout = new QVBoxLayout(widget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout = mainLayout;

    m_accGroupBox = new QGroupBox(i18n("Accounts"), this);
    mainLayout->addWidget(m_accGroupBox);
    QHBoxLayout *accLayout = new QHBoxLayout(m_accGroupBox);

    m_accComboBox = new QComboBox(m_accGroupBox);
    accLayout->addWidget(m_accComboBox, 1);
    connect(m_accComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &GoogleSettingsDialog::currentAccountChanged);

    m_addAccButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add-user")), i18n("&Add"), m_accGroupBox);
    accLayout->addWidget(m_addAccButton);
    connect(m_addAccButton, &QPushButton::clicked, this, &GoogleSettingsDialog::slotAddAccountClicked);

    m_removeAccButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove-user")), i18n("&Remove"), m_accGroupBox);
    accLayout->addWidget(m_removeAccButton);
    connect(m_removeAccButton, &QPushButton::clicked, this, &GoogleSettingsDialog::slotRemoveAccountClicked);

    QGroupBox *refreshBox = new QGroupBox(i18n("Refresh"), this);
    mainLayout->addWidget(refreshBox);
    QGridLayout *refreshLayout = new QGridLayout(refreshBox);

    m_enableRefresh = new QCheckBox(i18n("Enable interval refresh"), refreshBox);
    m_enableRefresh->setChecked(Settings::self()->enableIntervalCheck());
    refreshLayout->addWidget(m_enableRefresh, 0, 0, 1, 2);

    QLabel *label = new QLabel(i18n("Refresh interval:"));
    refreshLayout->addWidget(label, 1, 0);
    m_refreshSpinBox = new KPluralHandlingSpinBox(this);
    m_refreshSpinBox->setMaximum(720);
    m_refreshSpinBox->setMinimum(10);
    m_refreshSpinBox->setSingleStep(1);
    m_refreshSpinBox->setValue(30);
    m_refreshSpinBox->setDisplayIntegerBase(10);
    m_refreshSpinBox->setSuffix(ki18np(" minute", " minutes"));
    m_refreshSpinBox->setEnabled(Settings::self()->enableIntervalCheck());
    refreshLayout->addWidget(m_refreshSpinBox, 1, 1);
    connect(m_enableRefresh, &QCheckBox::toggled, m_refreshSpinBox, &KPluralHandlingSpinBox::setEnabled);

    if (m_enableRefresh->isEnabled()) {
        m_refreshSpinBox->setValue(Settings::self()->intervalCheckTime());
    }
    QMetaObject::invokeMethod(this, &GoogleSettingsDialog::reloadAccounts, Qt::QueuedConnection);
}

GoogleSettingsDialog::~GoogleSettingsDialog()
{
}

QVBoxLayout *GoogleSettingsDialog::mainLayout() const
{
    return m_mainLayout;
}

GoogleAccountManager *GoogleSettingsDialog::accountManager() const
{
    return m_accountManager;
}

KGAPI2::AccountPtr GoogleSettingsDialog::currentAccount() const
{
    return m_accountManager->findAccount(m_accComboBox->currentText());
}

void GoogleSettingsDialog::reloadAccounts()
{
    disconnect(m_accComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &GoogleSettingsDialog::currentAccountChanged);

    m_accComboBox->clear();

    const AccountsList accounts = m_accountManager->listAccounts();
    for (const AccountPtr &account : accounts) {
        m_accComboBox->addItem(account->accountName());
    }

    int index = m_accComboBox->findText(m_parentResource->settings()->account(), Qt::MatchExactly);
    if (index > -1) {
        m_accComboBox->setCurrentIndex(index);
    }

    connect(m_accComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &GoogleSettingsDialog::currentAccountChanged);

    m_removeAccButton->setEnabled(m_accComboBox->count() > 0);
    Q_EMIT currentAccountChanged(m_accComboBox->currentText());
}

void GoogleSettingsDialog::slotAddAccountClicked()
{
    AccountPtr account(new Account());
    // FIXME: We need a proper API for this
    account->addScope(Account::contactsScopeUrl());
    account->addScope(Account::calendarScopeUrl());
    account->addScope(Account::tasksScopeUrl());
    account->addScope(Account::accountInfoEmailScopeUrl());
    account->addScope(Account::accountInfoScopeUrl());

    AuthJob *authJob = new AuthJob(account,
                                   m_parentResource->settings()->clientId(),
                                   m_parentResource->settings()->clientSecret());
    connect(authJob, &AuthJob::finished, this, &GoogleSettingsDialog::slotAccountAuthenticated);
}

void GoogleSettingsDialog::slotRemoveAccountClicked()
{
    const AccountPtr account = currentAccount();
    if (!account) {
        return;
    }

    if (KMessageBox::warningYesNo(
            this,
            i18n("Do you really want to revoke access to account <b>%1</b>?"
                 "<p>This will revoke access to all resources using this account!</p>",
                 account->accountName()),
            i18n("Revoke Access?"),
            KStandardGuiItem::yes(),
            KStandardGuiItem::no(),
            QString(),
            KMessageBox::Dangerous) != KMessageBox::Yes) {
        return;
    }

    m_accountManager->removeAccount(account->accountName());
    reloadAccounts();
}

void GoogleSettingsDialog::slotAccountAuthenticated(Job *job)
{
    AuthJob *authJob = qobject_cast<AuthJob *>(job);
    const AccountPtr account = authJob->account();

    if (authJob->error() != KGAPI2::NoError) {
        KMessageBox::sorry(this, authJob->errorString());
        return;
    }

    if (!m_accountManager->storeAccount(account)) {
        qWarning() << "Failed to add account to KWallet";
    }

    reloadAccounts();
}

bool GoogleSettingsDialog::handleError(Job *job)
{
    if ((job->error() == KGAPI2::NoError) || (job->error() == KGAPI2::OK)) {
        return true;
    }

    if (job->error() == KGAPI2::Unauthorized) {
        qDebug() << job << job->errorString();
        const AccountPtr account = currentAccount();
        const QList<QUrl> resourceScopes = m_parentResource->scopes();
        for (const QUrl &scope : resourceScopes) {
            if (!account->scopes().contains(scope)) {
                account->addScope(scope);
            }
        }

        AuthJob *authJob = new AuthJob(account, m_parentResource->settings()->clientId(),
                                       m_parentResource->settings()->clientSecret(), this);
        authJob->setProperty(JOB_PROPERTY, QVariant::fromValue(job));
        connect(authJob, &AuthJob::finished, this, &GoogleSettingsDialog::slotAuthJobFinished);

        return false;
    }

    KMessageBox::sorry(this, job->errorString());
    job->deleteLater();
    return false;
}

void GoogleSettingsDialog::slotAuthJobFinished(Job *job)
{
    qDebug();

    if (job->error() != KGAPI2::NoError) {
        KMessageBox::sorry(this, job->errorString());
        return;
    }

    AuthJob *authJob = qobject_cast<AuthJob *>(job);
    const AccountPtr account = authJob->account();
    if (!m_accountManager->storeAccount(account)) {
        qWarning() << "Failed to store account in KWallet";
    }

    KGAPI2::Job *otherJob = job->property(JOB_PROPERTY).value<KGAPI2::Job *>();
    otherJob->setAccount(account);
    otherJob->restart();

    job->deleteLater();
}

void GoogleSettingsDialog::slotSaveSettings()
{
    Settings::self()->setEnableIntervalCheck(m_enableRefresh->isChecked());
    Settings::self()->setIntervalCheckTime(m_refreshSpinBox->value());

    saveSettings();
    accept();
}
