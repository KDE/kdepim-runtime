/* This file is part of the KDE project

   Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                      a KDAB Group company <info@kdab.com>
   Author: Kevin Ottens <kevin@kdab.com>

   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
   Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
   Copyright (C) 2009 Kevin Ottens <ervin@kde.org>
   Copyright (C) 2006-2008 Omat Holding B.V. <info@omat.nl>
   Copyright (C) 2006 Frode M. Døving <frode@lnix.net>

   Original copied from showfoto:
    Copyright 2005 by Gilles Caulier <caulier.gilles@free.fr>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "setupserver.h"
#include "settings.h"
#include "imapresource.h"
#include "serverinfodialog.h"
#include "folderarchivesettingpage.h"
#include "utils.h"

#include <mailtransport/transport.h>
#include <mailtransport/servertest.h>

#include <kmime/kmime_message.h>

#include <collectionfetchjob.h>
#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/KMime/SpecialMailCollectionsRequestJob>
#include <resourcesettings.h>
#include <entitydisplayattribute.h>
#include <CollectionModifyJob>
#include <kemailsettings.h>
#include <KLocalizedString>
#include <qpushbutton.h>
#include <kmessagebox.h>
#include <kuser.h>
#include "imapresource_debug.h"
#include <QNetworkConfigurationManager>

#include <kidentitymanagement/identitymanager.h>
#include <kidentitymanagement/identitycombo.h>
#include <QVBoxLayout>

#include "imapaccount.h"
#include "subscriptiondialog.h"

#include "ui_setupserverview_desktop.h"

/** static helper functions **/
static QString authenticationModeString(MailTransport::Transport::EnumAuthenticationType::type mode)
{
    switch (mode) {
    case  MailTransport::Transport::EnumAuthenticationType::LOGIN:
        return QStringLiteral("LOGIN");
    case MailTransport::Transport::EnumAuthenticationType::PLAIN:
        return QStringLiteral("PLAIN");
    case MailTransport::Transport::EnumAuthenticationType::CRAM_MD5:
        return QStringLiteral("CRAM-MD5");
    case MailTransport::Transport::EnumAuthenticationType::DIGEST_MD5:
        return QStringLiteral("DIGEST-MD5");
    case MailTransport::Transport::EnumAuthenticationType::GSSAPI:
        return QStringLiteral("GSSAPI");
    case MailTransport::Transport::EnumAuthenticationType::NTLM:
        return QStringLiteral("NTLM");
    case MailTransport::Transport::EnumAuthenticationType::CLEAR:
        return i18nc("Authentication method", "Clear text");
    case MailTransport::Transport::EnumAuthenticationType::ANONYMOUS:
        return i18nc("Authentication method", "Anonymous");
    case MailTransport::Transport::EnumAuthenticationType::XOAUTH2:
        return i18nc("Authentication method", "Gmail");
    default:
        break;
    }
    return QString();
}

static void setCurrentAuthMode(QComboBox *authCombo, MailTransport::Transport::EnumAuthenticationType::type authtype)
{
    qCDebug(IMAPRESOURCE_LOG) << "setting authcombo: " << authenticationModeString(authtype);
    int index = authCombo->findData(authtype);
    if (index == -1) {
        qCWarning(IMAPRESOURCE_LOG) << "desired authmode not in the combo";
    }
    qCDebug(IMAPRESOURCE_LOG) << "found corresponding index: " << index << "with data" << authenticationModeString((MailTransport::Transport::EnumAuthenticationType::type) authCombo->itemData(index).toInt());
    authCombo->setCurrentIndex(index);
    MailTransport::Transport::EnumAuthenticationType::type t = (MailTransport::Transport::EnumAuthenticationType::type) authCombo->itemData(authCombo->currentIndex()).toInt();
    qCDebug(IMAPRESOURCE_LOG) << "selected auth mode:" << authenticationModeString(t);
    Q_ASSERT(t == authtype);
}

static MailTransport::Transport::EnumAuthenticationType::type getCurrentAuthMode(QComboBox *authCombo)
{
    MailTransport::Transport::EnumAuthenticationType::type authtype = (MailTransport::Transport::EnumAuthenticationType::type) authCombo->itemData(authCombo->currentIndex()).toInt();
    qCDebug(IMAPRESOURCE_LOG) << "current auth mode: " << authenticationModeString(authtype);
    return authtype;
}

static void addAuthenticationItem(QComboBox *authCombo, MailTransport::Transport::EnumAuthenticationType::type authtype)
{
    qCDebug(IMAPRESOURCE_LOG) << "adding auth item " << authenticationModeString(authtype);
    authCombo->addItem(authenticationModeString(authtype), QVariant(authtype));
}

SetupServer::SetupServer(ImapResourceBase *parentResource, WId parent)
    : QDialog(), m_parentResource(parentResource), m_ui(new Ui::SetupServerView), m_serverTest(nullptr),
      m_subscriptionsChanged(false), m_shouldClearCache(false), mValidator(this)
{
    QNetworkConfigurationManager *networkConfigMgr = new QNetworkConfigurationManager(QCoreApplication::instance());

    m_parentResource->settings()->setWinId(parent);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(mOkButton, &QPushButton::clicked, this, &SetupServer::applySettings);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SetupServer::reject);
    mainLayout->addWidget(buttonBox);

    m_ui->setupUi(mainWidget);
    m_folderArchiveSettingPage = new FolderArchiveSettingPage(m_parentResource->identifier());
    m_ui->tabWidget->addTab(m_folderArchiveSettingPage, i18n("Archive Folder"));
    m_ui->safeImapGroup->setId(m_ui->noRadio, KIMAP::LoginJob::Unencrypted);
    m_ui->safeImapGroup->setId(m_ui->sslRadio, KIMAP::LoginJob::AnySslVersion);
    m_ui->safeImapGroup->setId(m_ui->tlsRadio, KIMAP::LoginJob::TlsV1);

    connect(m_ui->noRadio, &QRadioButton::toggled, this, &SetupServer::slotSafetyChanged);
    connect(m_ui->sslRadio, &QRadioButton::toggled, this, &SetupServer::slotSafetyChanged);
    connect(m_ui->tlsRadio, &QRadioButton::toggled, this, &SetupServer::slotSafetyChanged);

    m_ui->testInfo->hide();
    m_ui->testProgress->hide();
    m_ui->accountName->setFocus();
    m_ui->checkInterval->setSuffix(ki18np(" minute", " minutes"));
    m_ui->checkInterval->setMinimum(Akonadi::ResourceSettings::self()->minimumCheckInterval());
    m_ui->checkInterval->setMaximum(10000);
    m_ui->checkInterval->setSingleStep(1);

    // regex for evaluating a valid server name/ip
    mValidator.setRegExp(QRegExp(QLatin1String("[A-Za-z0-9-_:.]*")));
    m_ui->imapServer->setValidator(&mValidator);

    m_ui->folderRequester->setMimeTypeFilter(
        QStringList() << KMime::Message::mimeType());
    m_ui->folderRequester->setAccessRightsFilter(Akonadi::Collection::CanChangeItem | Akonadi::Collection::CanCreateItem | Akonadi::Collection::CanDeleteItem);
    m_ui->folderRequester->changeCollectionDialogOptions(Akonadi::CollectionDialog::AllowToCreateNewChildCollection);
    m_identityManager = new KIdentityManagement::IdentityManager(false, this, "mIdentityManager");
    m_identityCombobox = new KIdentityManagement::IdentityCombo(m_identityManager, this);
    m_ui->identityLabel->setBuddy(m_identityCombobox);
    m_ui->identityLayout->addWidget(m_identityCombobox, 1);
    m_ui->identityLabel->setBuddy(m_identityCombobox);

    connect(m_ui->testButton, &QPushButton::pressed, this, &SetupServer::slotTest);

    connect(m_ui->imapServer, &KLineEdit::textChanged, this, &SetupServer::slotServerChanged);
    connect(m_ui->imapServer, &KLineEdit::textChanged, this, &SetupServer::slotTestChanged);
    connect(m_ui->imapServer, &KLineEdit::textChanged, this, &SetupServer::slotComplete);
    connect(m_ui->userName, &KLineEdit::textChanged, this, &SetupServer::slotComplete);
    connect(m_ui->subscriptionEnabled, &QCheckBox::toggled, this, &SetupServer::slotSubcriptionCheckboxChanged);
    connect(m_ui->subscriptionButton, &QPushButton::pressed, this, &SetupServer::slotManageSubscriptions);

    connect(m_ui->managesieveCheck, &QCheckBox::toggled, this, &SetupServer::slotEnableWidgets);
    connect(m_ui->sameConfigCheck, &QCheckBox::toggled, this, &SetupServer::slotEnableWidgets);

    connect(m_ui->useDefaultIdentityCheck, &QCheckBox::toggled, this, &SetupServer::slotIdentityCheckboxChanged);
    connect(m_ui->enableMailCheckBox, &QCheckBox::toggled, this, &SetupServer::slotMailCheckboxChanged);
    connect(m_ui->safeImapGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SetupServer::slotEncryptionRadioChanged);
    connect(m_ui->customSieveGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SetupServer::slotCustomSieveChanged);
    connect(m_ui->showServerInfo, &QPushButton::pressed, this, &SetupServer::slotShowServerInfo);

    readSettings();
    slotTestChanged();
    slotComplete();
    slotCustomSieveChanged();
    connect(networkConfigMgr, &QNetworkConfigurationManager::onlineStateChanged,
            this, &SetupServer::slotTestChanged);
}

SetupServer::~SetupServer()
{
    delete m_ui;
}

bool SetupServer::shouldClearCache() const
{
    return m_shouldClearCache;
}

void SetupServer::slotSubcriptionCheckboxChanged()
{
    m_ui->subscriptionButton->setEnabled(m_ui->subscriptionEnabled->isChecked());
}

void SetupServer::slotIdentityCheckboxChanged()
{
    m_identityCombobox->setEnabled(!m_ui->useDefaultIdentityCheck->isChecked());
}

void SetupServer::slotMailCheckboxChanged()
{
    m_ui->checkInterval->setEnabled(m_ui->enableMailCheckBox->isChecked());
}

void SetupServer::slotEncryptionRadioChanged()
{
    // TODO these really should be defined somewhere else
    switch (m_ui->safeImapGroup->checkedId()) {
    case KIMAP::LoginJob::Unencrypted:
    case KIMAP::LoginJob::TlsV1:
        m_ui->portSpin->setValue(143);
        break;
    case KIMAP::LoginJob::AnySslVersion:
        m_ui->portSpin->setValue(993);
        break;
    default:
        qFatal("Shouldn't happen");
    }
}

void SetupServer::slotCustomSieveChanged()
{
    QAbstractButton *checkedButton = m_ui->customSieveGroup->checkedButton();

    if (checkedButton == m_ui->imapUserPassword ||
            checkedButton == m_ui->noAuthentification) {
        m_ui->customUsername->setEnabled(false);
        m_ui->customPassword->setEnabled(false);
    } else if (checkedButton == m_ui->customUserPassword) {
        m_ui->customUsername->setEnabled(true);
        m_ui->customPassword->setEnabled(true);
    }
}

void SetupServer::applySettings()
{
    if (!m_parentResource->settings()->imapServer().isEmpty()
        && m_ui->imapServer->text() != m_parentResource->settings()->imapServer()) {
        if (KMessageBox::warningContinueCancel(
                this, i18n("You have changed the address of the server. Even if this is the same server as before "
                           "we will have to re-download all your emails from this account again. "
                           "Are you sure you want to proceed?"),
                i18n("Server address change")) == KMessageBox::Cancel) {
            return;
        }
    }
    if (!m_parentResource->settings()->userName().isEmpty()
        && m_ui->userName->text() != m_parentResource->settings()->userName()) {
        if (KMessageBox::warningContinueCancel(
                this, i18n("You have changed the user name. Even if this is a user name for the same account as before "
                           "we will have to re-download all your emails from this account again. "
                           "Are you sure you want to proceed?"),
                i18n("User name change")) == KMessageBox::Cancel) {
            return;
        }
    }

    m_folderArchiveSettingPage->writeSettings();
    m_shouldClearCache = (m_parentResource->settings()->imapServer() != m_ui->imapServer->text())
                         || (m_parentResource->settings()->userName() != m_ui->userName->text());

    m_parentResource->setName(m_ui->accountName->text());

    m_parentResource->settings()->setImapServer(m_ui->imapServer->text());
    m_parentResource->settings()->setImapPort(m_ui->portSpin->value());
    m_parentResource->settings()->setUserName(m_ui->userName->text());
    QString encryption;
    switch (m_ui->safeImapGroup->checkedId()) {
    case KIMAP::LoginJob::Unencrypted :
        encryption = QStringLiteral("None");
        break;
    case KIMAP::LoginJob::AnySslVersion:
        encryption = QStringLiteral("SSL");
        break;
    case KIMAP::LoginJob::TlsV1:
        encryption = QStringLiteral("STARTTLS");
        break;
    default:
        qFatal("Shouldn't happen");
    }
    m_parentResource->settings()->setSafety(encryption);
    MailTransport::Transport::EnumAuthenticationType::type authtype = getCurrentAuthMode(m_ui->authenticationCombo);
    qCDebug(IMAPRESOURCE_LOG) << "saving IMAP auth mode: " << authenticationModeString(authtype);
    m_parentResource->settings()->setAuthentication(authtype);
    m_parentResource->settings()->setPassword(m_ui->password->text());
    m_parentResource->settings()->setSubscriptionEnabled(m_ui->subscriptionEnabled->isChecked());
    m_parentResource->settings()->setIntervalCheckTime(m_ui->checkInterval->value());
    m_parentResource->settings()->setDisconnectedModeEnabled(m_ui->disconnectedModeEnabled->isChecked());

    MailTransport::Transport::EnumAuthenticationType::type alternateAuthtype = getCurrentAuthMode(m_ui->authenticationAlternateCombo);
    qCDebug(IMAPRESOURCE_LOG) << "saving Alternate server sieve auth mode: " << authenticationModeString(alternateAuthtype);
    m_parentResource->settings()->setAlternateAuthentication(alternateAuthtype);
    m_parentResource->settings()->setSieveSupport(m_ui->managesieveCheck->isChecked());
    m_parentResource->settings()->setSieveReuseConfig(m_ui->sameConfigCheck->isChecked());
    m_parentResource->settings()->setSievePort(m_ui->sievePortSpin->value());
    m_parentResource->settings()->setSieveAlternateUrl(m_ui->alternateURL->text());
    m_parentResource->settings()->setSieveVacationFilename(m_vacationFileName);

    m_parentResource->settings()->setTrashCollection(m_ui->folderRequester->collection().id());
    Akonadi::Collection trash = m_ui->folderRequester->collection();
    Akonadi::SpecialMailCollections::self()->registerCollection(Akonadi::SpecialMailCollections::Trash, trash);
    Akonadi::EntityDisplayAttribute *attribute =  trash.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
    attribute->setIconName(QStringLiteral("user-trash"));
    new Akonadi::CollectionModifyJob(trash);
    if (mOldTrash != trash) {
        Akonadi::SpecialMailCollections::self()->unregisterCollection(mOldTrash);
    }

    m_parentResource->settings()->setAutomaticExpungeEnabled(m_ui->autoExpungeCheck->isChecked());
    m_parentResource->settings()->setUseDefaultIdentity(m_ui->useDefaultIdentityCheck->isChecked());

    if (!m_ui->useDefaultIdentityCheck->isChecked()) {
        m_parentResource->settings()->setAccountIdentity(m_identityCombobox->currentIdentity());
    }

    m_parentResource->settings()->setIntervalCheckEnabled(m_ui->enableMailCheckBox->isChecked());
    if (m_ui->enableMailCheckBox->isChecked()) {
        m_parentResource->settings()->setIntervalCheckTime(m_ui->checkInterval->value());
    }

    m_parentResource->settings()->setSieveCustomUsername(m_ui->customUsername->text());

    QAbstractButton *checkedButton = m_ui->customSieveGroup->checkedButton();

    if (checkedButton == m_ui->imapUserPassword) {
        m_parentResource->settings()->setSieveCustomAuthentification(QStringLiteral("ImapUserPassword"));
    } else if (checkedButton == m_ui->noAuthentification) {
        m_parentResource->settings()->setSieveCustomAuthentification(QStringLiteral("NoAuthentification"));
    } else if (checkedButton == m_ui->customUserPassword) {
        m_parentResource->settings()->setSieveCustomAuthentification(QStringLiteral("CustomUserPassword"));
    }

    m_parentResource->settings()->setSieveCustomPassword(m_ui->customPassword->text());

    m_parentResource->settings()->save();
    qCDebug(IMAPRESOURCE_LOG) << "wrote" << m_ui->imapServer->text() << m_ui->userName->text() << m_ui->safeImapGroup->checkedId();

    if (m_oldResourceName != m_ui->accountName->text() && !m_ui->accountName->text().isEmpty()) {
        m_parentResource->settings()->renameRootCollection(m_ui->accountName->text());
    }

    accept();
}

void SetupServer::readSettings()
{
    m_folderArchiveSettingPage->loadSettings();
    m_ui->accountName->setText(m_parentResource->name());
    m_oldResourceName = m_ui->accountName->text();

    KUser *currentUser = new KUser();
    KEMailSettings esetting;

    m_ui->imapServer->setText(
        !m_parentResource->settings()->imapServer().isEmpty() ? m_parentResource->settings()->imapServer() :
        esetting.getSetting(KEMailSettings::InServer));

    m_ui->portSpin->setValue(m_parentResource->settings()->imapPort());

    m_ui->userName->setText(
        !m_parentResource->settings()->userName().isEmpty() ? m_parentResource->settings()->userName() :
        currentUser->loginName());

    const QString safety = m_parentResource->settings()->safety();
    int i = 0;
    if (safety == QLatin1String("SSL")) {
        i = KIMAP::LoginJob::AnySslVersion;
    } else if (safety == QLatin1String("STARTTLS")) {
        i = KIMAP::LoginJob::TlsV1;
    } else {
        i = KIMAP::LoginJob::Unencrypted;
    }

    QAbstractButton *safetyButton = m_ui->safeImapGroup->button(i);
    if (safetyButton) {
        safetyButton->setChecked(true);
    }

    populateDefaultAuthenticationOptions();
    i = m_parentResource->settings()->authentication();
    qCDebug(IMAPRESOURCE_LOG) << "read IMAP auth mode: " << authenticationModeString((MailTransport::Transport::EnumAuthenticationType::type) i);
    setCurrentAuthMode(m_ui->authenticationCombo, (MailTransport::Transport::EnumAuthenticationType::type) i);

    i = m_parentResource->settings()->alternateAuthentication();
    setCurrentAuthMode(m_ui->authenticationAlternateCombo, (MailTransport::Transport::EnumAuthenticationType::type) i);

    bool rejected = false;
    const QString password = m_parentResource->settings()->password(&rejected);
    if (rejected) {
        m_ui->password->setEnabled(false);
        KMessageBox::information(nullptr, i18n("Could not access KWallet. "
                                 "If you want to store the password permanently then you have to "
                                 "activate it. If you do not "
                                 "want to use KWallet, check the box below, but note that you will be "
                                 "prompted for your password when needed."),
                                 i18n("Do not use KWallet"), QStringLiteral("warning_kwallet_disabled"));
    } else {
        m_ui->password->insert(password);
    }

    m_ui->subscriptionEnabled->setChecked(m_parentResource->settings()->subscriptionEnabled());

    m_ui->checkInterval->setValue(m_parentResource->settings()->intervalCheckTime());
    m_ui->disconnectedModeEnabled->setChecked(m_parentResource->settings()->disconnectedModeEnabled());

    m_ui->managesieveCheck->setChecked(m_parentResource->settings()->sieveSupport());
    m_ui->sameConfigCheck->setChecked(m_parentResource->settings()->sieveReuseConfig());
    m_ui->sievePortSpin->setValue(m_parentResource->settings()->sievePort());
    m_ui->alternateURL->setText(m_parentResource->settings()->sieveAlternateUrl());
    m_vacationFileName = m_parentResource->settings()->sieveVacationFilename();

    Akonadi::Collection trashCollection(m_parentResource->settings()->trashCollection());
    if (trashCollection.isValid()) {
        Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(trashCollection, Akonadi::CollectionFetchJob::Base, this);
        connect(fetchJob, &Akonadi::CollectionFetchJob::collectionsReceived, this, &SetupServer::targetCollectionReceived);
    } else {
        Akonadi::SpecialMailCollectionsRequestJob *requestJob = new Akonadi::SpecialMailCollectionsRequestJob(this);
        connect(requestJob, &Akonadi::SpecialMailCollectionsRequestJob::result, this, &SetupServer::localFolderRequestJobFinished);
        requestJob->requestDefaultCollection(Akonadi::SpecialMailCollections::Trash);
        requestJob->start();
    }

    m_ui->useDefaultIdentityCheck->setChecked(m_parentResource->settings()->useDefaultIdentity());
    if (!m_ui->useDefaultIdentityCheck->isChecked()) {
        m_identityCombobox->setCurrentIdentity(m_parentResource->settings()->accountIdentity());
    }

    m_ui->enableMailCheckBox->setChecked(m_parentResource->settings()->intervalCheckEnabled());
    if (m_ui->enableMailCheckBox->isChecked()) {
        m_ui->checkInterval->setValue(m_parentResource->settings()->intervalCheckTime());
    } else {
        m_ui->checkInterval->setEnabled(false);
    }

    m_ui->autoExpungeCheck->setChecked(m_parentResource->settings()->automaticExpungeEnabled());

    if (m_vacationFileName.isEmpty()) {
        m_vacationFileName = QStringLiteral("kmail-vacation.siv");
    }

    m_ui->customUsername->setText(m_parentResource->settings()->sieveCustomUsername());

    rejected = false;
    const QString customPassword = m_parentResource->settings()->sieveCustomPassword(&rejected);
    if (rejected) {
        m_ui->customPassword->setEnabled(false);
        KMessageBox::information(nullptr, i18n("Could not access KWallet. "
                                 "If you want to store the password permanently then you have to "
                                 "activate it. If you do not "
                                 "want to use KWallet, check the box below, but note that you will be "
                                 "prompted for your password when needed."),
                                 i18n("Do not use KWallet"), QStringLiteral("warning_kwallet_disabled"));
    } else {
        m_ui->customPassword->insert(customPassword);
    }

    const QString sieverCustomAuth(m_parentResource->settings()->sieveCustomAuthentification());
    if (sieverCustomAuth == QLatin1String("ImapUserPassword")) {
        m_ui->imapUserPassword->setChecked(true);
    } else if (sieverCustomAuth == QLatin1String("NoAuthentification")) {
        m_ui->noAuthentification->setChecked(true);
    } else if (sieverCustomAuth == QLatin1String("CustomUserPassword")) {
        m_ui->customUserPassword->setChecked(true);
    }

    delete currentUser;
}

void SetupServer::slotTest()
{
    qCDebug(IMAPRESOURCE_LOG) << m_ui->imapServer->text();

    m_ui->testButton->setEnabled(false);
    m_ui->safeImap->setEnabled(false);
    m_ui->authenticationCombo->setEnabled(false);

    m_ui->testInfo->clear();
    m_ui->testInfo->hide();

    delete m_serverTest;
    m_serverTest = new MailTransport::ServerTest(this);
#ifndef QT_NO_CURSOR
    qApp->setOverrideCursor(Qt::BusyCursor);
#endif

    const QString server = m_ui->imapServer->text();
    const int port = m_ui->portSpin->value();
    qCDebug(IMAPRESOURCE_LOG) << "server: " << server << "port: " << port;

    m_serverTest->setServer(server);

    if (port != 143 && port != 993) {
        m_serverTest->setPort(MailTransport::Transport::EnumEncryption::None, port);
        m_serverTest->setPort(MailTransport::Transport::EnumEncryption::SSL, port);
    }

    m_serverTest->setProtocol(QStringLiteral("imap"));
    m_serverTest->setProgressBar(m_ui->testProgress);
    connect(m_serverTest, &MailTransport::ServerTest::finished, this, &SetupServer::slotFinished);
    mOkButton->setEnabled(false);
    m_serverTest->start();
}

void SetupServer::slotFinished(const QVector<int> &testResult)
{
    qCDebug(IMAPRESOURCE_LOG) << testResult;

#ifndef QT_NO_CURSOR
    qApp->restoreOverrideCursor();
#endif
    mOkButton->setEnabled(true);

    using namespace MailTransport;

    if (!m_serverTest->isNormalPossible() && !m_serverTest->isSecurePossible()) {
        KMessageBox::sorry(this, i18n("Unable to connect to the server, please verify the server address."));
    }

    m_ui->testInfo->show();

    m_ui->sslRadio->setEnabled(testResult.contains(Transport::EnumEncryption::SSL));
    m_ui->tlsRadio->setEnabled(testResult.contains(Transport::EnumEncryption::TLS));
    m_ui->noRadio->setEnabled(testResult.contains(Transport::EnumEncryption::None));

    QString text;
    if (testResult.contains(Transport::EnumEncryption::TLS)) {
        m_ui->tlsRadio->setChecked(true);
        text = i18n("<qt><b>TLS is supported and recommended.</b></qt>");
    } else if (testResult.contains(Transport::EnumEncryption::SSL)) {
        m_ui->sslRadio->setChecked(true);
        text = i18n("<qt><b>SSL is supported and recommended.</b></qt>");
    } else if (testResult.contains(Transport::EnumEncryption::None)) {
        m_ui->noRadio->setChecked(true);
        text = i18n("<qt><b>No security is supported. It is not "
                    "recommended to connect to this server.</b></qt>");
    } else {
        text = i18n("<qt><b>It is not possible to use this server.</b></qt>");
    }
    m_ui->testInfo->setText(text);

    m_ui->testButton->setEnabled(true);
    m_ui->safeImap->setEnabled(true);
    m_ui->authenticationCombo->setEnabled(true);
    slotEncryptionRadioChanged();
    slotSafetyChanged();
}

void SetupServer::slotTestChanged()
{
    delete m_serverTest;
    m_serverTest = nullptr;
    slotSafetyChanged();

#ifdef WITH_GMAIL_XOAUTH2
    // do not use imapConnectionPossible, as the data is not yet saved.
    const bool isGmail = Utils::isGmail(m_ui->imapServer->text());
    m_ui->testButton->setEnabled(!isGmail /* TODO && Global::connectionPossible() ||
                                                m_ui->imapServer->text() == "localhost"*/);
#endif
}

void SetupServer::slotEnableWidgets()
{
    const bool haveSieve = m_ui->managesieveCheck->isChecked();
    const bool reuseConfig = m_ui->sameConfigCheck->isChecked();

    m_ui->sameConfigCheck->setEnabled(haveSieve);
    m_ui->sievePortSpin->setEnabled(haveSieve);
    m_ui->alternateURL->setEnabled(haveSieve && !reuseConfig);
    m_ui->authentification->setEnabled(haveSieve && !reuseConfig);
}

void SetupServer::slotComplete()
{
    const bool ok =  !m_ui->imapServer->text().isEmpty() && !m_ui->userName->text().isEmpty();
    mOkButton->setEnabled(ok);
}

void SetupServer::slotSafetyChanged()
{
    if (m_serverTest == nullptr) {
#ifdef WITH_GMAIL_XOAUTH2
        const bool isGmail = Utils::isGmail(m_ui->imapServer->text());
        qCDebug(IMAPRESOURCE_LOG) << "serverTest null";
        m_ui->noRadio->setEnabled(!isGmail);
        m_ui->sslRadio->setEnabled(!isGmail);
        m_ui->tlsRadio->setEnabled(!isGmail);

        m_ui->authenticationCombo->setEnabled(!isGmail);
#endif
        return;
    }
    QVector<int> protocols;

    switch (m_ui->safeImapGroup->checkedId()) {
    case KIMAP::LoginJob::Unencrypted :
        qCDebug(IMAPRESOURCE_LOG) << "safeImapGroup: unencrypted";
        protocols = m_serverTest->normalProtocols();
        break;
    case KIMAP::LoginJob::AnySslVersion:
        protocols = m_serverTest->secureProtocols();
        qCDebug(IMAPRESOURCE_LOG) << "safeImapGroup: SSL";
        break;
    case KIMAP::LoginJob::TlsV1:
        protocols = m_serverTest->tlsProtocols();
        qCDebug(IMAPRESOURCE_LOG) << "safeImapGroup: starttls";
        break;
    default:
        qFatal("Shouldn't happen");
    }

    m_ui->authenticationCombo->clear();
    addAuthenticationItem(m_ui->authenticationCombo, MailTransport::Transport::EnumAuthenticationType::CLEAR);
    for (int prot : qAsConst(protocols)) {
        addAuthenticationItem(m_ui->authenticationCombo, (MailTransport::Transport::EnumAuthenticationType::type) prot);
    }
    if (protocols.isEmpty()) {
        qCDebug(IMAPRESOURCE_LOG) << "no authmodes found";
    } else {
        setCurrentAuthMode(m_ui->authenticationCombo, (MailTransport::Transport::EnumAuthenticationType::type) protocols.first());
    }
}

void SetupServer::slotManageSubscriptions()
{
    qCDebug(IMAPRESOURCE_LOG) << "manage subscripts";
    ImapAccount account;

    account.setServer(m_ui->imapServer->text());
    account.setPort(m_ui->portSpin->value());

    account.setUserName(m_ui->userName->text());
    account.setSubscriptionEnabled(m_ui->subscriptionEnabled->isChecked());

    account.setEncryptionMode(
        static_cast<KIMAP::LoginJob::EncryptionMode>(m_ui->safeImapGroup->checkedId())
    );

    account.setAuthenticationMode(Settings::mapTransportAuthToKimap(getCurrentAuthMode(m_ui->authenticationCombo)));

    QPointer<SubscriptionDialog> subscriptions = new SubscriptionDialog(this);
    subscriptions->setWindowTitle(i18n("Serverside Subscription"));
    subscriptions->setWindowIcon(QIcon::fromTheme(QStringLiteral("network-server")));
    subscriptions->connectAccount(account, m_ui->password->text());
    m_subscriptionsChanged = subscriptions->isSubscriptionChanged();

    subscriptions->exec();
    delete subscriptions;

    m_ui->subscriptionEnabled->setChecked(account.isSubscriptionEnabled());
}

void SetupServer::slotShowServerInfo()
{
    ServerInfoDialog *dialog = new ServerInfoDialog(m_parentResource, this);
    dialog->show();
}

void SetupServer::targetCollectionReceived(const Akonadi::Collection::List &collections)
{
    m_ui->folderRequester->setCollection(collections.first());
    mOldTrash = collections.first();
}

void SetupServer::localFolderRequestJobFinished(KJob *job)
{
    if (!job->error()) {
        Akonadi::Collection targetCollection = Akonadi::SpecialMailCollections::self()->defaultCollection(Akonadi::SpecialMailCollections::Trash);
        Q_ASSERT(targetCollection.isValid());
        m_ui->folderRequester->setCollection(targetCollection);
        mOldTrash = targetCollection;
    }
}

void SetupServer::populateDefaultAuthenticationOptions()
{
    populateDefaultAuthenticationOptions(m_ui->authenticationCombo);
    populateDefaultAuthenticationOptions(m_ui->authenticationAlternateCombo);
}

void SetupServer::populateDefaultAuthenticationOptions(QComboBox *combo)
{
    combo->clear();
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::CLEAR);
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::LOGIN);
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::PLAIN);
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::CRAM_MD5);
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::DIGEST_MD5);
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::NTLM);
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::GSSAPI);
    addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::ANONYMOUS);
#ifdef WITH_GMAIL_XOAUTH2
    if (Utils::isGmail(m_ui->imapServer->text())) {
        addAuthenticationItem(combo, MailTransport::Transport::EnumAuthenticationType::XOAUTH2);
    }
#endif
}

void SetupServer::slotServerChanged()
{
#ifdef WITH_GMAIL_XOAUTH2
    const bool isGmail = Utils::isGmail(m_ui->imapServer->text());
    const bool wasGmail = !m_ui->password->isEnabled();

    if (isGmail == wasGmail) {
        return;
    }

    m_ui->password->setEnabled(!isGmail);
    m_ui->managesieveCheck->setChecked(m_ui->managesieveCheck->isChecked() && !isGmail);
    m_ui->managesieveCheck->setEnabled(!isGmail);
    m_ui->testButton->setEnabled(!isGmail);
    m_ui->sslRadio->setChecked(m_ui->sslRadio->isChecked() || isGmail);
    m_ui->noRadio->setEnabled(!isGmail);
    m_ui->sslRadio->setEnabled(!isGmail);
    m_ui->tlsRadio->setEnabled(!isGmail);
    m_ui->portSpin->setValue(isGmail ? 993 : m_ui->portSpin->value());
    m_ui->portSpin->setEnabled(!isGmail);
    populateDefaultAuthenticationOptions(m_ui->authenticationCombo);
    if (isGmail) {
        setCurrentAuthMode(m_ui->authenticationCombo, MailTransport::Transport::EnumAuthenticationType::XOAUTH2);
    }
    m_ui->authenticationCombo->setEnabled(!isGmail);
#endif
}
