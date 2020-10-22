/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "setupwizard.h"

#include <KLocalizedString>
#include <KPasswordLineEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>

#include "etesync_debug.h"
#include "loginjob.h"

using namespace EteSyncAPI;

SetupWizard::SetupWizard(EteSyncClientState *clientState, QWidget *parent)
    : QWizard(parent)
    , mClientState(clientState)
{
    setWindowTitle(i18nc("@title:window", "EteSync configuration wizard"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("akonadi-etesync")));
    setPage(W_LoginPage, new LoginPage);
    setPage(W_EncryptionPasswordPage, new EncryptionPasswordPage);

    disconnect(button(QWizard::NextButton), SIGNAL(clicked()), this, SLOT(next()));
    connect(button(QWizard::NextButton), SIGNAL(clicked()), this, SLOT(manualNext()));
    disconnect(button(QWizard::FinishButton), SIGNAL(clicked()), this, SLOT(accept()));
    connect(button(QWizard::FinishButton), SIGNAL(clicked()), this, SLOT(manualNext()));
}

void SetupWizard::manualNext()
{
    qCDebug(ETESYNC_LOG) << "Manual next";
    if (currentId() == W_LoginPage) {
        static_cast<LoginPage *>(page(W_LoginPage))->showProgressBar();
        const QString username = field(QStringLiteral("credentialsUserName")).toString();
        const QString password = field(QStringLiteral("credentialsPassword")).toString();
        const QString advancedServerUrl = field(QStringLiteral("credentialsServerUrl")).toString();
        const QString serverUrl = advancedServerUrl.isEmpty() ? QStringLiteral("https://api.etesync.com") : advancedServerUrl;
        auto job = new LoginJob(mClientState, serverUrl, username, password, this);
        connect(job, &LoginJob::finished, this, [this](KJob *job) {
            qCDebug(ETESYNC_LOG) << "Login finished";
            static_cast<LoginPage *>(page(W_LoginPage))->setLoginResult(static_cast<LoginJob *>(job)->getLoginResult());
            static_cast<LoginPage *>(page(W_LoginPage))->setUserInfoResult(static_cast<LoginJob *>(job)->getUserInfoResult());
            static_cast<LoginPage *>(page(W_LoginPage))->setErrorCode(job->error());
            static_cast<LoginPage *>(page(W_LoginPage))->setErrorMessage(job->errorText());
            static_cast<LoginPage *>(page(W_LoginPage))->hideProgressBar();
            nextId() == -1 ? QWizard::accept() : QWizard::next();
        });
        job->start();
        return;
    }
    nextId() == -1 ? QWizard::accept() : QWizard::next();
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

    mProgressBar = new QProgressBar;
    mProgressBar->setVisible(false);
    mProgressBar->setRange(0, 0);
    layout->addWidget(mProgressBar);

    connect(mAdvancedSettings, SIGNAL(toggled(bool)), mServerUrl, SLOT(setVisible(bool)));
    connect(mAdvancedSettings, SIGNAL(toggled(bool)), layout->labelForField(mServerUrl), SLOT(setVisible(bool)));
}

void LoginPage::initializePage()
{
    mIsInitialized = static_cast<SetupWizard *>(wizard())->mClientState->isInitialized();
    qCDebug(ETESYNC_LOG) << "Login page - isInitialized" << mIsInitialized;
    if (mIsInitialized) {
        mAdvancedSettings->setVisible(false);
        setField(QStringLiteral("credentialsServerUrl"), static_cast<SetupWizard *>(wizard())->mClientState->serverUrl());
        QString username = static_cast<SetupWizard *>(wizard())->mClientState->username();
        mUserName->setText(username);
    }
}

int LoginPage::nextId() const
{
    if (mIsInitialized) {
        return -1;
    }
    return SetupWizard::W_EncryptionPasswordPage;
}

bool LoginPage::validatePage()
{
    if (!mLoginResult) {
        qCDebug(ETESYNC_LOG) << "loginResult error" << mErrorCode;
        if (mErrorCode == EteSyncErrorCode::ETESYNC_ERROR_CODE_UNAUTHORIZED || mErrorCode == EteSyncErrorCode::ETESYNC_ERROR_CODE_HTTP) {
            mLoginLabel->setText(i18n("Incorrect login credentials. Please try again."));
        } else if (mErrorCode == EteSyncErrorCode::ETESYNC_ERROR_CODE_ENCODING) {
            mLoginLabel->setText(i18n("Please ensure that the server URL is correct. The URL should start with http:// or https://."));
        } else if (mErrorCode == EteSyncErrorCode::ETESYNC_ERROR_CODE_CONNECTION) {
            mLoginLabel->setText(i18n("Could not connect to the server. Please ensure that the server URL is correct."));
        } else {
            mLoginLabel->setText(i18n(charArrFromQString(mErrorMessage)));
        }
        return false;
    }

    if (!mUserInfoResult) {
        if (mErrorCode == EteSyncErrorCode::ETESYNC_ERROR_CODE_NOT_FOUND) {
            wizard()->setProperty("initAccount", true);
            return true;
        }
        mLoginLabel->setText(i18n(charArrFromQString(mErrorMessage)));
        return false;
    }
    return true;
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
    const auto initAccount = wizard()->property("initAccount");
    if (initAccount.isValid() && initAccount.toBool()) {
        setSubTitle(i18n("Welcome to EteSync! Please set your encryption password below, and make sure you got it right, as it can't be recovered if lost!"));
    }
}

bool EncryptionPasswordPage::validatePage()
{
    const QString encryptionPassword = field(QStringLiteral("credentialsEncryptionPassword")).toString();
    const auto initAccount = wizard()->property("initAccount");
    if (initAccount.isValid() && initAccount.toBool()) {
        return static_cast<SetupWizard *>(wizard())->mClientState->initAccount(encryptionPassword);
    }
    const bool keypairResult = static_cast<SetupWizard *>(wizard())->mClientState->initKeypair(encryptionPassword);
    if (!keypairResult) {
        if (etesync_get_error_code() == ETESYNC_ERROR_CODE_ENCRYPTION_MAC) {
            mEncryptionPasswordLabel->setText(i18n("Incorrect encryption password. Please try again."));
        } else {
            mEncryptionPasswordLabel->setText(i18n(CharPtr(etesync_get_error_message()).get()));
        }
    }
    return keypairResult;
}
