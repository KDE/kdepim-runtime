/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "setupwizard.h"

#include <KLocalizedString>
#include <KPasswordLineEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "etesync_debug.h"

SetupWizard::SetupWizard(EteSyncClientState *clientState, QWidget *parent)
    : QWizard(parent), mClientState(clientState)
{
    setWindowTitle(i18nc("@title:window", "EteSync configuration wizard"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("akonadi-etesync")));
    setPage(W_LoginPage, new LoginPage);
    setPage(W_EncryptionPasswordPage, new EncryptionPasswordPage);
}

LoginPage::LoginPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18n("Login Credentials"));
    setSubTitle(i18n("Enter your credentials to login to the EteSync server"));

    QFormLayout *layout = new QFormLayout(this);

    mLoginLabel = new QLabel;
    mLoginLabel->setWordWrap(true);
    layout->addWidget(mLoginLabel);

    mUserName = new QLineEdit;
    layout->addRow(i18n("Email:"), mUserName);
    registerField(QStringLiteral("credentialsUserName*"), mUserName);

    mPassword = new KPasswordLineEdit;
    layout->addRow(i18n("Password:"), mPassword);
    registerField(QStringLiteral("credentialsPassword*"), mPassword, "password", SIGNAL(passwordChanged(QString)));

    mAdvancedSettings = new QCheckBox(i18n("Advanced settings"), this);
    layout->addWidget(mAdvancedSettings);

    mServerUrl = new QLineEdit;
    mServerUrl->setVisible(false);
    layout->addRow(i18n("Server:"), mServerUrl);
    registerField(QStringLiteral("credentialsServerUrl"), mServerUrl);

    layout->labelForField(mServerUrl)->setVisible(false);

    connect(mAdvancedSettings, SIGNAL(toggled(bool)), mServerUrl, SLOT(setVisible(bool)));
    connect(mAdvancedSettings, SIGNAL(toggled(bool)), layout->labelForField(mServerUrl), SLOT(setVisible(bool)));
}

int LoginPage::nextId() const
{
    // Implement token job
    return SetupWizard::W_EncryptionPasswordPage;
}

bool LoginPage::validatePage()
{
    const QString username = field(QStringLiteral("credentialsUserName")).toString();
    const QString password = field(QStringLiteral("credentialsPassword")).toString();
    const QString advancedServerUrl = field(QStringLiteral("credentialsServerUrl")).toString();
    const QString serverUrl = advancedServerUrl.isEmpty() ? QStringLiteral("https://api.etesync.com") : advancedServerUrl;
    const bool loginResult = static_cast<SetupWizard *>(wizard())->mClientState->initToken(serverUrl, username, password);
    if (!loginResult) {
        mLoginLabel->setText(i18n("Incorrect login credentials. Please try again."));
    }
    return loginResult;
}

EncryptionPasswordPage::EncryptionPasswordPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18n("Encryption password"));
    setSubTitle(i18n("Enter the encryption password for your EteSync account"));

    QFormLayout *layout = new QFormLayout(this);

    mEncryptionPasswordLabel = new QLabel;
    mEncryptionPasswordLabel->setWordWrap(true);
    layout->addWidget(mEncryptionPasswordLabel);

    mEncryptionPassword = new KPasswordLineEdit;
    layout->addRow(i18n("Encryption Password:"), mEncryptionPassword);
    registerField(QStringLiteral("credentialsEncryptionPassword*"), mEncryptionPassword, "password", SIGNAL(passwordChanged(QString)));
}

int EncryptionPasswordPage::nextId() const
{
    return -1;
}

void EncryptionPasswordPage::initializePage()
{
    const bool userInfoResult = static_cast<SetupWizard *>(wizard())->mClientState->initUserInfo();
    if (!userInfoResult) {
        setSubTitle(i18n("Please set your encryption password below, and make sure you got it right, as it can't be recovered if lost!"));
        mInitAccount = true;
    }
}

bool EncryptionPasswordPage::validatePage()
{
    const QString encryptionPassword = field(QStringLiteral("credentialsEncryptionPassword")).toString();
    if (mInitAccount) {
        static_cast<SetupWizard *>(wizard())->mClientState->initAccount(encryptionPassword);
        return true;
    }
    const bool keypairResult = static_cast<SetupWizard *>(wizard())->mClientState->initKeypair(encryptionPassword);
    if (!keypairResult) {
        mEncryptionPasswordLabel->setText(i18n("Incorrect encryption password. Please try again."));
    }
    return keypairResult;
}