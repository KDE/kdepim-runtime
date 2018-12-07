/*
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright 2009 Thomas McGuire <mcguire@kde.org>
 *   Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Local includes
#include "accountdialog.h"
#include "pop3resource.h"
#include "settings.h"
#include "settingsadaptor.h"

// KDEPIMLIBS includes
#include <Collection>
#include <CollectionFetchJob>
#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/KMime/SpecialMailCollectionsRequestJob>
#include <resourcesettings.h>
#include <MailTransport/ServerTest>

// KDELIBS includes
#include <KEMailSettings>
#include <KMessageBox>
#include <KUser>
#include <KWindowSystem>
#include <kwallet.h>
#include "pop3resource_debug.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace MailTransport;
using namespace Akonadi;
using namespace KWallet;

namespace {
class BusyCursorHelper : public QObject
{
public:
    inline BusyCursorHelper(QObject *parent)
        : QObject(parent)
    {
#ifndef QT_NO_CURSOR
        qApp->setOverrideCursor(Qt::BusyCursor);
#endif
    }

    inline ~BusyCursorHelper()
    {
#ifndef QT_NO_CURSOR
        qApp->restoreOverrideCursor();
#endif
    }
};
}

AccountDialog::AccountDialog(POP3Resource *parentResource, WId parentWindow)
    : QDialog()
    , mParentResource(parentResource)
    , mServerTest(nullptr)
    , mValidator(this)
    , mWallet(nullptr)
{
    KWindowSystem::setMainWindow(this, parentWindow);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("network-server")));
    setWindowTitle(i18n("POP3 Account Settings"));
    mValidator.setRegExp(QRegExp(QLatin1String("[A-Za-z0-9-_:.]*")));

    setupWidgets();
    loadSettings();
}

AccountDialog::~AccountDialog()
{
    delete mWallet;
    mWallet = nullptr;
    delete mServerTest;
    mServerTest = nullptr;
}

void AccountDialog::setupWidgets()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AccountDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AccountDialog::reject);

    QWidget *page = new QWidget(this);
    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);

    setupUi(page);

    // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
    // compatibility) are allowed
    hostEdit->setValidator(&mValidator);
    intervalSpin->setSuffix(ki18np(" minute", " minutes"));

    intervalSpin->setRange(ResourceSettings::self()->minimumCheckInterval(), 10000);
    intervalSpin->setSingleStep(1);

    connect(leaveOnServerCheck, &QCheckBox::clicked, this, &AccountDialog::slotLeaveOnServerClicked);
    connect(leaveOnServerDaysCheck, &QCheckBox::toggled, this, &AccountDialog::slotEnableLeaveOnServerDays);
    connect(leaveOnServerDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AccountDialog::slotLeaveOnServerDaysChanged);
    connect(leaveOnServerCountCheck, &QCheckBox::toggled, this, &AccountDialog::slotEnableLeaveOnServerCount);
    connect(leaveOnServerCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AccountDialog::slotLeaveOnServerCountChanged);
    connect(leaveOnServerSizeCheck, &QCheckBox::toggled, this, &AccountDialog::slotEnableLeaveOnServerSize);

    connect(filterOnServerSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AccountDialog::slotFilterOnServerSizeChanged);
    connect(filterOnServerCheck, &QCheckBox::toggled, filterOnServerSizeSpin, &QSpinBox::setEnabled);
    connect(filterOnServerCheck, &QCheckBox::clicked, this, &AccountDialog::slotFilterOnServerClicked);

    connect(checkCapabilities, &QPushButton::clicked, this, &AccountDialog::slotCheckPopCapabilities);
    encryptionButtonGroup = new QButtonGroup(this);
    encryptionButtonGroup->addButton(encryptionNone,
                                     Transport::EnumEncryption::None);
    encryptionButtonGroup->addButton(encryptionSSL,
                                     Transport::EnumEncryption::SSL);
    encryptionButtonGroup->addButton(encryptionTLS,
                                     Transport::EnumEncryption::TLS);

    connect(encryptionButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &AccountDialog::slotPopEncryptionChanged);
    connect(intervalCheck, &QCheckBox::toggled, this, &AccountDialog::slotEnablePopInterval);

    populateDefaultAuthenticationOptions();

    folderRequester->setMimeTypeFilter(
        QStringList() << QStringLiteral("message/rfc822"));
    folderRequester->setAccessRightsFilter(Akonadi::Collection::CanCreateItem);
    folderRequester->changeCollectionDialogOptions(Akonadi::CollectionDialog::AllowToCreateNewChildCollection);

    connect(usePipeliningCheck, &QCheckBox::clicked, this, &AccountDialog::slotPipeliningClicked);

    // FIXME: Hide widgets which are not supported yet
    filterOnServerCheck->hide();
    filterOnServerSizeSpin->hide();
}

void AccountDialog::loadSettings()
{
    if (mParentResource->name() == mParentResource->identifier()) {
        mParentResource->setName(i18n("POP3 Account"));
    }

    nameEdit->setText(mParentResource->name());
    nameEdit->setFocus();
    loginEdit->setText(!Settings::self()->login().isEmpty() ? Settings::self()->login()
                       : KUser().loginName());

    hostEdit->setText(
        !Settings::self()->host().isEmpty() ? Settings::self()->host()
        : KEMailSettings().getSetting(KEMailSettings::InServer));
    hostEdit->setText(Settings::self()->host());
    portEdit->setValue(Settings::self()->port());
    precommand->setText(Settings::self()->precommand());
    usePipeliningCheck->setChecked(Settings::self()->pipelining());
    leaveOnServerCheck->setChecked(Settings::self()->leaveOnServer());
    leaveOnServerDaysCheck->setEnabled(Settings::self()->leaveOnServer());
    leaveOnServerDaysCheck->setChecked(Settings::self()->leaveOnServerDays() >= 1);
    leaveOnServerDaysSpin->setValue(Settings::self()->leaveOnServerDays() >= 1
                                    ? Settings::self()->leaveOnServerDays() : 7);
    leaveOnServerCountCheck->setEnabled(Settings::self()->leaveOnServer());
    leaveOnServerCountCheck->setChecked(Settings::self()->leaveOnServerCount() >= 1);
    leaveOnServerCountSpin->setValue(Settings::self()->leaveOnServerCount() >= 1
                                     ? Settings::self()->leaveOnServerCount() : 100);
    leaveOnServerSizeCheck->setEnabled(Settings::self()->leaveOnServer());
    leaveOnServerSizeCheck->setChecked(Settings::self()->leaveOnServerSize() >= 1);
    leaveOnServerSizeSpin->setValue(Settings::self()->leaveOnServerSize() >= 1
                                    ? Settings::self()->leaveOnServerSize() : 10);
    filterOnServerCheck->setChecked(Settings::self()->filterOnServer());
    filterOnServerSizeSpin->setValue(Settings::self()->filterCheckSize());
    intervalCheck->setChecked(Settings::self()->intervalCheckEnabled());
    intervalSpin->setValue(Settings::self()->intervalCheckInterval());
    intervalSpin->setEnabled(Settings::self()->intervalCheckEnabled());

    const int authenticationMethod = Settings::self()->authenticationMethod();
    authCombo->setCurrentIndex(authCombo->findData(authenticationMethod));
    encryptionNone->setChecked(!Settings::self()->useSSL() && !Settings::self()->useTLS());
    encryptionSSL->setChecked(Settings::self()->useSSL());
    encryptionTLS->setChecked(Settings::self()->useTLS());
    proxyCheck->setChecked(Settings::self()->useProxy());

    slotEnableLeaveOnServerDays(leaveOnServerDaysCheck->isEnabled()
                                ? Settings::self()->leaveOnServerDays() >= 1 : false);
    slotEnableLeaveOnServerCount(leaveOnServerCountCheck->isEnabled()
                                 ? Settings::self()->leaveOnServerCount() >= 1 : false);
    slotEnableLeaveOnServerSize(leaveOnServerSizeCheck->isEnabled()
                                ? Settings::self()->leaveOnServerSize() >= 1 : false);

    // We need to fetch the collection, as the CollectionRequester needs the name
    // of it to work correctly
    Collection targetCollection(Settings::self()->targetCollection());
    if (targetCollection.isValid()) {
        CollectionFetchJob *fetchJob = new CollectionFetchJob(targetCollection,
                                                              CollectionFetchJob::Base,
                                                              this);
        connect(fetchJob, &CollectionFetchJob::collectionsReceived, this, &AccountDialog::targetCollectionReceived);
    } else {
        // FIXME: This is a bit duplicated from POP3Resource...

        // No target collection set in the config? Try requesting a default inbox
        SpecialMailCollectionsRequestJob *requestJob = new SpecialMailCollectionsRequestJob(this);
        requestJob->requestDefaultCollection(SpecialMailCollections::Inbox);
        requestJob->start();
        connect(requestJob, &SpecialMailCollectionsRequestJob::result, this, &AccountDialog::localFolderRequestJobFinished);
    }

    mWallet = Wallet::openWallet(Wallet::NetworkWallet(), winId(),
                                 Wallet::Asynchronous);
    if (mWallet) {
        connect(mWallet, &KWallet::Wallet::walletOpened, this, &AccountDialog::walletOpenedForLoading);
    } else {
        passwordEdit->lineEdit()->setPlaceholderText(i18n("Wallet disabled in system settings"));
    }
    passwordEdit->setEnabled(false);
    passwordLabel->setEnabled(false);
}

void AccountDialog::walletOpenedForLoading(bool success)
{
    if (success) {
        if (mWallet->isOpen()) {
            passwordEdit->setEnabled(true);
            passwordLabel->setEnabled(true);
        }
        if (mWallet->isOpen() && mWallet->hasFolder(QStringLiteral("pop3"))) {
            QString password;
            mWallet->setFolder(QStringLiteral("pop3"));
            mWallet->readPassword(mParentResource->identifier(), password);
            passwordEdit->setPassword(password);
            mInitalPassword = password;
        } else {
            qCWarning(POP3RESOURCE_LOG) << "Wallet not open or doesn't have pop3 folder.";
        }
    } else {
        qCWarning(POP3RESOURCE_LOG) << "Failed to open wallet for loading the password.";
    }

    const bool walletError = !success || !mWallet->isOpen();
    if (walletError) {
        passwordEdit->lineEdit()->setPlaceholderText(i18n("Unable to open wallet"));
    }
}

void AccountDialog::walletOpenedForSaving(bool success)
{
    if (success) {
        if (mWallet && mWallet->isOpen()) {
            // Remove the password from the wallet if the user doesn't want to store it
            if (passwordEdit->password().isEmpty() && mWallet->hasFolder(QStringLiteral("pop3"))) {
                mWallet->setFolder(QStringLiteral("pop3"));
                mWallet->removeEntry(mParentResource->identifier());
            }
            // Store the password in the wallet if the user wants that
            else if (!passwordEdit->password().isEmpty()) {
                if (!mWallet->hasFolder(QStringLiteral("pop3"))) {
                    mWallet->createFolder(QStringLiteral("pop3"));
                }
                mWallet->setFolder(QStringLiteral("pop3"));
                mWallet->writePassword(mParentResource->identifier(), passwordEdit->password());
            }

            mParentResource->clearCachedPassword();
        } else {
            qCWarning(POP3RESOURCE_LOG) << "Wallet not open.";
        }
    } else {
        // Should we alert the user here?
        qCWarning(POP3RESOURCE_LOG) << "Failed to open wallet for saving the password.";
    }

    delete mWallet;
    mWallet = nullptr;
    accept();
}

void AccountDialog::slotLeaveOnServerClicked()
{
    const bool state = leaveOnServerCheck->isChecked();
    leaveOnServerDaysCheck->setEnabled(state);
    leaveOnServerCountCheck->setEnabled(state);
    leaveOnServerSizeCheck->setEnabled(state);
    if (state) {
        if (leaveOnServerDaysCheck->isChecked()) {
            slotEnableLeaveOnServerDays(state);
        }
        if (leaveOnServerCountCheck->isChecked()) {
            slotEnableLeaveOnServerCount(state);
        }
        if (leaveOnServerSizeCheck->isChecked()) {
            slotEnableLeaveOnServerSize(state);
        }
    } else {
        slotEnableLeaveOnServerDays(state);
        slotEnableLeaveOnServerCount(state);
        slotEnableLeaveOnServerSize(state);
    }
    if (mServerTest && !mServerTest->capabilities().contains(ServerTest::UIDL)
        && leaveOnServerCheck->isChecked()) {
        KMessageBox::information(topLevelWidget(),
                                 i18n("The server does not seem to support unique "
                                      "message numbers, but this is a "
                                      "requirement for leaving messages on the "
                                      "server.\n"
                                      "Since some servers do not correctly "
                                      "announce their capabilities you still "
                                      "have the possibility to turn leaving "
                                      "fetched messages on the server on."));
    }
}

void AccountDialog::slotFilterOnServerClicked()
{
    if (mServerTest && !mServerTest->capabilities().contains(ServerTest::Top)
        && filterOnServerCheck->isChecked()) {
        KMessageBox::information(topLevelWidget(),
                                 i18n("The server does not seem to support "
                                      "fetching message headers, but this is a "
                                      "requirement for filtering messages on the "
                                      "server.\n"
                                      "Since some servers do not correctly "
                                      "announce their capabilities you still "
                                      "have the possibility to turn filtering "
                                      "messages on the server on."));
    }
}

void AccountDialog::slotPipeliningClicked()
{
    if (usePipeliningCheck->isChecked()) {
        KMessageBox::information(topLevelWidget(),
                                 i18n("Please note that this feature can cause some POP3 servers "
                                      "that do not support pipelining to send corrupted mail;\n"
                                      "this is configurable, though, because some servers support pipelining "
                                      "but do not announce their capabilities. To check whether your POP3 server "
                                      "announces pipelining support use the \"Auto Detect\""
                                      " button at the bottom of the dialog;\n"
                                      "if your server does not announce it, but you want more speed, then "
                                      "you should do some testing first by sending yourself a batch "
                                      "of mail and downloading it."), QString(),
                                 QStringLiteral("pipelining"));
    }
}

void AccountDialog::slotPopEncryptionChanged(int id)
{
    qCDebug(POP3RESOURCE_LOG) << "setting port";
    // adjust port
    if (id == Transport::EnumEncryption::SSL || portEdit->value() == 995) {
        portEdit->setValue((id == Transport::EnumEncryption::SSL) ? 995 : 110);
    }

    qCDebug(POP3RESOURCE_LOG) << "port set ";
    enablePopFeatures(); // removes invalid auth options from the combobox
}

void AccountDialog::slotCheckPopCapabilities()
{
    if (hostEdit->text().isEmpty()) {
        KMessageBox::sorry(this, i18n("Please specify a server and port on "
                                      "the General tab first."));
        return;
    }
    delete mServerTest;
    mServerTest = new ServerTest(this);
    BusyCursorHelper *busyCursorHelper = new BusyCursorHelper(mServerTest);
    mServerTest->setProgressBar(checkCapabilitiesProgress);
    mOkButton->setEnabled(false);
    checkCapabilitiesStack->setCurrentIndex(1);
    Transport::EnumEncryption::type encryptionType;
    if (encryptionSSL->isChecked()) {
        encryptionType = Transport::EnumEncryption::SSL;
    } else {
        encryptionType = Transport::EnumEncryption::None;
    }
    mServerTest->setPort(encryptionType, portEdit->value());
    mServerTest->setServer(hostEdit->text());
    mServerTest->setProtocol(QStringLiteral("pop"));
    connect(mServerTest, &MailTransport::ServerTest::finished, this, &AccountDialog::slotPopCapabilities);
    connect(mServerTest, &MailTransport::ServerTest::finished, busyCursorHelper, &BusyCursorHelper::deleteLater);

    mServerTest->start();
    mServerTestFailed = false;
}

void AccountDialog::slotPopCapabilities(const QVector<int> &encryptionTypes)
{
    checkCapabilitiesStack->setCurrentIndex(0);
    mOkButton->setEnabled(true);

    // if both fail, popup a dialog
    if (!mServerTest->isNormalPossible() && !mServerTest->isSecurePossible()) {
        KMessageBox::sorry(this, i18n("Unable to connect to the server, please verify the server address."));
    }

    // If the servertest did not find any useable authentication modes, assume the
    // connection failed and don't disable any of the radioboxes.
    if (encryptionTypes.isEmpty()) {
        mServerTestFailed = true;
        return;
    }

    encryptionNone->setEnabled(encryptionTypes.contains(Transport::EnumEncryption::None));
    encryptionSSL->setEnabled(encryptionTypes.contains(Transport::EnumEncryption::SSL));
    encryptionTLS->setEnabled(encryptionTypes.contains(Transport::EnumEncryption::TLS));

    usePipeliningCheck->setChecked(mServerTest->capabilities().contains(ServerTest::Pipelining));

    checkHighest(encryptionButtonGroup);
}

void AccountDialog::enablePopFeatures()
{
    if (!mServerTest || mServerTestFailed) {
        return;
    }

    QVector<int> supportedAuths;
    if (encryptionButtonGroup->checkedId() == Transport::EnumEncryption::None) {
        supportedAuths = mServerTest->normalProtocols();
    }
    if (encryptionButtonGroup->checkedId() == Transport::EnumEncryption::SSL) {
        supportedAuths = mServerTest->secureProtocols();
    }
    if (encryptionButtonGroup->checkedId() == Transport::EnumEncryption::TLS) {
        supportedAuths = mServerTest->tlsProtocols();
    }

    authCombo->clear();
    for (int prot : qAsConst(supportedAuths)) {
        authCombo->addItem(Transport::authenticationTypeString(prot), prot);
    }

    if (mServerTest && !mServerTest->capabilities().contains(ServerTest::Pipelining)
        && usePipeliningCheck->isChecked()) {
        usePipeliningCheck->setChecked(false);
        KMessageBox::information(topLevelWidget(),
                                 i18n("The server does not seem to support "
                                      "pipelining; therefore, this option has "
                                      "been disabled.\n"
                                      "Since some servers do not correctly "
                                      "announce their capabilities you still "
                                      "have the possibility to turn pipelining "
                                      "on. But please note that this feature can "
                                      "cause some POP servers that do not "
                                      "support pipelining to send corrupt "
                                      "messages. So before using this feature "
                                      "with important mail you should first "
                                      "test it by sending yourself a larger "
                                      "number of test messages which you all "
                                      "download in one go from the POP "
                                      "server."));
    }

    if (mServerTest && !mServerTest->capabilities().contains(ServerTest::UIDL)
        && leaveOnServerCheck->isChecked()) {
        leaveOnServerCheck->setChecked(false);
        KMessageBox::information(topLevelWidget(),
                                 i18n("The server does not seem to support unique "
                                      "message numbers, but this is a "
                                      "requirement for leaving messages on the "
                                      "server; therefore, this option has been "
                                      "disabled.\n"
                                      "Since some servers do not correctly "
                                      "announce their capabilities you still "
                                      "have the possibility to turn leaving "
                                      "fetched messages on the server on."));
    }

    if (mServerTest && !mServerTest->capabilities().contains(ServerTest::Top)
        && filterOnServerCheck->isChecked()) {
        filterOnServerCheck->setChecked(false);
        KMessageBox::information(topLevelWidget(),
                                 i18n("The server does not seem to support "
                                      "fetching message headers, but this is a "
                                      "requirement for filtering messages on the "
                                      "server; therefore, this option has been "
                                      "disabled.\n"
                                      "Since some servers do not correctly "
                                      "announce their capabilities you still "
                                      "have the possibility to turn filtering "
                                      "messages on the server on."));
    }
}

static void addAuthenticationItem(QComboBox *combo, int authenticationType)
{
    combo->addItem(Transport::authenticationTypeString(authenticationType),
                   QVariant(authenticationType));
}

void AccountDialog::populateDefaultAuthenticationOptions()
{
    authCombo->clear();
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::CLEAR);
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::LOGIN);
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::PLAIN);
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::CRAM_MD5);
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::DIGEST_MD5);
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::NTLM);
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::GSSAPI);
    addAuthenticationItem(authCombo, Transport::EnumAuthenticationType::APOP);
}

void AccountDialog::slotLeaveOnServerDaysChanged(int value)
{
    leaveOnServerDaysSpin->setSuffix(i18np(" day", " days", value));
}

void AccountDialog::slotLeaveOnServerCountChanged(int value)
{
    leaveOnServerCountSpin->setSuffix(i18np(" message", " messages", value));
}

void AccountDialog::slotFilterOnServerSizeChanged(int value)
{
    filterOnServerSizeSpin->setSuffix(i18np(" byte", " bytes", value));
}

void AccountDialog::checkHighest(QButtonGroup *btnGroup)
{
    QListIterator<QAbstractButton *> it(btnGroup->buttons());
    it.toBack();
    while (it.hasPrevious()) {
        QAbstractButton *btn = it.previous();
        if (btn && btn->isEnabled()) {
            btn->animateClick();
            return;
        }
    }
}

void AccountDialog::slotAccepted()
{
    saveSettings();
    // Don't call accept() yet, saveSettings() triggers an asnychronous wallet operation,
    // which will call accept() when it is finished
}

void AccountDialog::saveSettings()
{
    mParentResource->setName(nameEdit->text());

    Settings::self()->setIntervalCheckEnabled(intervalCheck->checkState() == Qt::Checked);
    Settings::self()->setIntervalCheckInterval(intervalSpin->value());
    Settings::self()->setHost(hostEdit->text().trimmed());
    Settings::self()->setPort(portEdit->value());
    Settings::self()->setLogin(loginEdit->text().trimmed());
    Settings::self()->setPrecommand(precommand->text());
    Settings::self()->setUseSSL(encryptionSSL->isChecked());
    Settings::self()->setUseTLS(encryptionTLS->isChecked());
    Settings::self()->setAuthenticationMethod(authCombo->itemData(authCombo->currentIndex()).toInt());
    Settings::self()->setUseProxy(proxyCheck->isChecked());
    Settings::self()->setPipelining(usePipeliningCheck->isChecked());
    Settings::self()->setLeaveOnServer(leaveOnServerCheck->isChecked());
    Settings::self()->setLeaveOnServerDays(leaveOnServerCheck->isChecked()
                                           ? (leaveOnServerDaysCheck->isChecked()
                                              ? leaveOnServerDaysSpin->value() : -1) : 0);
    Settings::self()->setLeaveOnServerCount(leaveOnServerCheck->isChecked()
                                            ? (leaveOnServerCountCheck->isChecked()
                                               ? leaveOnServerCountSpin->value() : -1) : 0);
    Settings::self()->setLeaveOnServerSize(leaveOnServerCheck->isChecked()
                                           ? (leaveOnServerSizeCheck->isChecked()
                                              ? leaveOnServerSizeSpin->value() : -1) : 0);
    Settings::self()->setFilterOnServer(filterOnServerCheck->isChecked());
    Settings::self()->setFilterCheckSize(filterOnServerSizeSpin->value());
    Settings::self()->setTargetCollection(folderRequester->collection().id());
    Settings::self()->save();

    // Now, either save the password or delete it from the wallet. For both, we need
    // to open it.
    const bool userChangedPassword = mInitalPassword != passwordEdit->password();
    const bool userWantsToDeletePassword
        = passwordEdit->password().isEmpty() && userChangedPassword;

    if ((!passwordEdit->password().isEmpty() && userChangedPassword)
        || userWantsToDeletePassword) {
        qCDebug(POP3RESOURCE_LOG) << mWallet <<  mWallet->isOpen();
        if (mWallet && mWallet->isOpen()) {
            // wallet is already open
            walletOpenedForSaving(true);
        } else {
            // we need to open the wallet
            qCDebug(POP3RESOURCE_LOG) << "we need to open the wallet";
            mWallet = Wallet::openWallet(Wallet::NetworkWallet(), winId(),
                                         Wallet::Asynchronous);
            if (mWallet) {
                connect(mWallet, &KWallet::Wallet::walletOpened, this, &AccountDialog::walletOpenedForSaving);
            } else {
                accept();
            }
        }
    } else {
        accept();
    }
}

void AccountDialog::slotEnableLeaveOnServerDays(bool state)
{
    if (state && !leaveOnServerDaysCheck->isEnabled()) {
        return;
    }
    leaveOnServerDaysSpin->setEnabled(state);
}

void AccountDialog::slotEnableLeaveOnServerCount(bool state)
{
    if (state && !leaveOnServerCountCheck->isEnabled()) {
        return;
    }
    leaveOnServerCountSpin->setEnabled(state);
}

void AccountDialog::slotEnableLeaveOnServerSize(bool state)
{
    if (state && !leaveOnServerSizeCheck->isEnabled()) {
        return;
    }
    leaveOnServerSizeSpin->setEnabled(state);
}

void AccountDialog::slotEnablePopInterval(bool state)
{
    intervalSpin->setEnabled(state);
    intervalLabel->setEnabled(state);
}

void AccountDialog::targetCollectionReceived(Akonadi::Collection::List collections)
{
    folderRequester->setCollection(collections.first());
}

void AccountDialog::localFolderRequestJobFinished(KJob *job)
{
    if (!job->error()) {
        Collection targetCollection = SpecialMailCollections::self()->defaultCollection(SpecialMailCollections::Inbox);
        Q_ASSERT(targetCollection.isValid());
        folderRequester->setCollection(targetCollection);
    }
}
