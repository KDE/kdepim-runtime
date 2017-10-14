/*
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "ewsconfigdialog.h"

#include <KConfigDialogManager>
#include <KWindowSystem>
#include <KMessageBox>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>


#include "ewsautodiscoveryjob.h"
#include "ewsgetfolderrequest.h"
#include "ewsresource.h"
#include "ewssettings.h"
#include "ewssubscriptionwidget.h"
#include "ewsprogressdialog.h"
#include "ui_ewsconfigdialog.h"

typedef QPair<QString, QString> StringPair;

static const QVector<StringPair> userAgents = {
    {QStringLiteral("Microsoft Outlook 2016"), QStringLiteral("Microsoft Office/16.0 (Windows NT 10.0; Microsoft Outlook 16.0.6326; Pro)")},
    {QStringLiteral("Microsoft Outlook 2013"), QStringLiteral("Microsoft Office/15.0 (Windows NT 6.1; Microsoft Outlook 15.0.4420; Pro)")},
    {QStringLiteral("Microsoft Outlook 2010"), QStringLiteral("Microsoft Office/14.0 (Windows NT 6.1; Microsoft Outlook 14.0.5128; Pro)")},
    {QStringLiteral("Microsoft Outlook 2011 for Mac"), QStringLiteral("MacOutlook/14.2.0.101115 (Intel Mac OS X 10.6.7)")},
    {QStringLiteral("Mozilla Thunderbird 38 for Windows (with ExQuilla)"), QStringLiteral("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.0) Gecko/20100101 Thunderbird/38.2.0")},
    {QStringLiteral("Mozilla Thunderbird 38 for Linux (with ExQuilla)"), QStringLiteral("Mozilla/5.0 (X11; Linux x86_64; rv:38.0) Gecko/20100101 Thunderbird/38.2.0")},
    {QStringLiteral("Mozilla Thunderbird 38 for Mac (with ExQuilla)"), QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:38.0) Gecko/20100101 Thunderbird/38.2.0")}
};

EwsConfigDialog::EwsConfigDialog(EwsResource *parentResource, EwsClient &client, WId wId)
    : QDialog()
    , mParentResource(parentResource)
{
    if (wId) {
        KWindowSystem::setMainWindow(this, wId);
    }

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &EwsConfigDialog::dialogAccepted);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &EwsConfigDialog::reject);
    mainLayout->addWidget(mButtonBox);

    setWindowTitle(i18n("Microsoft Exchange Configuration"));

    mUi = new Ui::SetupServerView;
    mUi->setupUi(mainWidget);
    mUi->accountName->setText(parentResource->name());

    mSubWidget = new EwsSubscriptionWidget(client, mParentResource->settings(), this);
    mUi->subscriptionTabLayout->addWidget(mSubWidget);

    mConfigManager = new KConfigDialogManager(this, mParentResource->settings());
    mConfigManager->updateWidgets();
    switch (mParentResource->settings()->retrievalMethod()) {
    case 0:
        mUi->pollRadioButton->setChecked(true);
        break;
    case 1:
        mUi->streamingRadioButton->setChecked(true);
        break;
    default:
        break;
    }

    const EwsServerVersion &serverVer = client.serverVersion();
    if (serverVer.isValid()) {
        mUi->serverStatusText->setText(i18nc("Server status", "OK"));
        mUi->serverVersionText->setText(serverVer.toString());
    }

    bool baseUrlEmpty = mUi->kcfg_BaseUrl->text().isEmpty();
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!baseUrlEmpty);
    mUi->tryConnectButton->setEnabled(!baseUrlEmpty);
    mTryConnectNeeded = baseUrlEmpty;

    QString password;
    mParentResource->settings()->requestPassword(password, false);
    mUi->passwordEdit->setPassword(password);

    int selectedIndex = -1;
    int i = 0;
    Q_FOREACH (const StringPair &item, userAgents) {
        mUi->userAgentCombo->addItem(item.first, item.second);
        if (mParentResource->settings()->userAgent() == item.second) {
            selectedIndex = i;
        }
        i++;
    }
    mUi->userAgentCombo->addItem(i18nc("User Agent", "Custom"));
    if (!mParentResource->settings()->userAgent().isEmpty()) {
        mUi->userAgentGroupBox->setChecked(true);
        mUi->userAgentCombo->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : mUi->userAgentCombo->count() - 1);
        mUi->userAgentEdit->setText(mParentResource->settings()->userAgent());
    } else {
        mUi->userAgentCombo->setCurrentIndex(mUi->userAgentCombo->count());
    }

    QIcon ewsIcon = QIcon::fromTheme(QStringLiteral("akonadi-ews"));
    mUi->aboutIconLabel->setPixmap(ewsIcon.pixmap(96, 96, QIcon::Normal, QIcon::On));
    mUi->aboutTextLabel->setText(i18nc("@info", "Akonadi Resource for Microsoft Exchange Web Services (EWS)"));
    mUi->aboutCopyrightLabel->setText(i18nc("@info", "Copyright (c) Krzysztof Nowicki 2015-2017"));
    mUi->aboutVersionLabel->setText(i18nc("@info", "Version %1", QStringLiteral(AKONADI_EWS_VERSION)));
    mUi->aboutLicenseLabel->setText(i18nc("@info", "Distributed under the GNU Library General Public License version 2.0 or later."));
    mUi->aboutUrlLabel->setText(QStringLiteral("<a href=\"https://github.com/KrissN/akonadi-ews\">https://github.com/KrissN/akonadi-ews</a>"));

    connect(okButton, &QPushButton::clicked, this, &EwsConfigDialog::save);
    connect(mUi->autodiscoverButton, &QPushButton::clicked, this, &EwsConfigDialog::performAutoDiscovery);
    connect(mUi->kcfg_Username, &KLineEdit::textChanged, this, &EwsConfigDialog::setAutoDiscoveryNeeded);
    connect(mUi->passwordEdit, &KPasswordLineEdit::passwordChanged, this, &EwsConfigDialog::setAutoDiscoveryNeeded);
    connect(mUi->kcfg_Domain, &KLineEdit::textChanged, this, &EwsConfigDialog::setAutoDiscoveryNeeded);
    connect(mUi->kcfg_HasDomain, &QCheckBox::toggled, this, &EwsConfigDialog::setAutoDiscoveryNeeded);
    connect(mUi->kcfg_Email, &KLineEdit::textChanged, this, &EwsConfigDialog::setAutoDiscoveryNeeded);
    connect(mUi->kcfg_BaseUrl, &KLineEdit::textChanged, this, &EwsConfigDialog::enableTryConnect);
    connect(mUi->tryConnectButton, &QPushButton::clicked, this, &EwsConfigDialog::tryConnect);
    connect(mUi->userAgentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EwsConfigDialog::userAgentChanged);
    connect(mUi->clearFolderTreeSyncStateButton, &QPushButton::clicked, mParentResource,
            &EwsResource::clearFolderTreeSyncState);
    connect(mUi->clearFolderItemSyncStateButton, &QPushButton::clicked, mParentResource,
            static_cast<void (EwsResource::*)()>(&EwsResource::clearFolderSyncState));
}

EwsConfigDialog::~EwsConfigDialog()
{
    delete mUi;
}

void EwsConfigDialog::save()
{
    mParentResource->setName(mUi->accountName->text());
    mConfigManager->updateSettings();
    if (mUi->pollRadioButton->isChecked()) {
        mParentResource->settings()->setRetrievalMethod(0);
    } else {
        mParentResource->settings()->setRetrievalMethod(1);
    }

    /* Erase the subscription id in case subscription is disabled or its parameters changed. This
     * fill force creation of a new subscription. */
    if (!mSubWidget->subscriptionEnabled() ||
        (mSubWidget->subscribedList() != mParentResource->settings()->serverSubscriptionList())) {
        mParentResource->settings()->setEventSubscriptionId(QString());
        mParentResource->settings()->setEventSubscriptionWatermark(QString());
    }

    mParentResource->settings()->setServerSubscription(mSubWidget->subscriptionEnabled());
    if (mSubWidget->subscribedListValid()) {
        mParentResource->settings()->setServerSubscriptionList(mSubWidget->subscribedList());
    }

    if (mUi->userAgentGroupBox->isChecked()) {
        mParentResource->settings()->setUserAgent(mUi->userAgentEdit->text());
    } else {
        mParentResource->settings()->setUserAgent(QString());
    }

    mParentResource->settings()->setPassword(mUi->passwordEdit->password());
    mParentResource->settings()->save();
}

void EwsConfigDialog::performAutoDiscovery()
{
    mAutoDiscoveryJob = new EwsAutodiscoveryJob(mUi->kcfg_Email->text(),
            fullUsername(), mUi->passwordEdit->password(),
            mUi->userAgentGroupBox->isEnabled() ? mUi->userAgentEdit->text() : QString(),
            mUi->kcfg_EnableNTLMv2->isChecked(), this);
    connect(mAutoDiscoveryJob, &EwsAutodiscoveryJob::result, this, &EwsConfigDialog::autoDiscoveryFinished);
    mProgressDialog = new EwsProgressDialog(this, EwsProgressDialog::AutoDiscovery);
    connect(mProgressDialog, &QDialog::rejected, this, &EwsConfigDialog::autoDiscoveryCancelled);
    mAutoDiscoveryJob->start();
    mProgressDialog->show();
}

void EwsConfigDialog::autoDiscoveryFinished(KJob *job)
{
    if (job->error() || job != mAutoDiscoveryJob) {
        KMessageBox::error(this, job->errorText(), i18nc("Exchange server autodiscovery", "Autodiscovery failed"));
        mProgressDialog->reject();
    } else {
        mProgressDialog->accept();
        mUi->kcfg_BaseUrl->setText(mAutoDiscoveryJob->ewsUrl());
    }
    mAutoDiscoveryJob->deleteLater();
    mAutoDiscoveryJob = nullptr;
    mProgressDialog->deleteLater();
    mProgressDialog = nullptr;
}

void EwsConfigDialog::tryConnectFinished(KJob *job)
{
    if (job->error() || job != mTryConnectJob) {
        KMessageBox::error(this, job->errorText(), i18nc("Exchange server connection", "Connection failed"));
        mUi->serverStatusText->setText(i18nc("Exchange server status", "Failed"));
        mUi->serverVersionText->setText(i18nc("Exchange server version", "Unknown"));
        mProgressDialog->reject();
    } else {
        mUi->serverStatusText->setText(i18nc("Exchange server status", "OK"));
        mUi->serverVersionText->setText(mTryConnectJob->serverVersion().toString());
        mProgressDialog->accept();
    }
    //mTryConnectJob->deleteLater();
    mTryConnectJob = nullptr;
    //mProgressDialog->deleteLater();
    mProgressDialog = nullptr;
}

void EwsConfigDialog::autoDiscoveryCancelled()
{
    if (mAutoDiscoveryJob) {
        mAutoDiscoveryJob->kill();
    }
    mProgressDialog->deleteLater();
    mProgressDialog = nullptr;
}

void EwsConfigDialog::tryConnectCancelled()
{
    if (mTryConnectJob) {
        mTryConnectJob->kill();
    }
    //mTryConnectJob->deleteLater();
    mTryConnectJob = nullptr;
}

void EwsConfigDialog::setAutoDiscoveryNeeded()
{
    mAutoDiscoveryNeeded = true;
    mTryConnectNeeded = true;

    /* Enable the OK button when at least the e-mail and username fields are set. Additionally if
     * autodiscovery is disabled, enable the OK button only if the base URL is set. */
    bool okEnabled = !mUi->kcfg_Username->text().isEmpty() && !mUi->kcfg_Email->text().isEmpty();
    if (!mUi->kcfg_AutoDiscovery->isChecked() && mUi->kcfg_BaseUrl->text().isEmpty()) {
        okEnabled = false;
    }
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
}

QString EwsConfigDialog::fullUsername() const
{
    QString username = mUi->kcfg_Username->text();
    if (mUi->kcfg_HasDomain->isChecked()) {
        username.prepend(mUi->kcfg_Domain->text() + QStringLiteral("\\"));
    }
    return username;
}

void EwsConfigDialog::dialogAccepted()
{
    if (mUi->kcfg_AutoDiscovery->isChecked() && mAutoDiscoveryNeeded) {
        mAutoDiscoveryJob = new EwsAutodiscoveryJob(mUi->kcfg_Email->text(),
                fullUsername(), mUi->passwordEdit->password(),
                mUi->userAgentGroupBox->isEnabled() ? mUi->userAgentEdit->text() : QString(),
                mUi->kcfg_EnableNTLMv2->isChecked(), this);
        connect(mAutoDiscoveryJob, &EwsAutodiscoveryJob::result, this, &EwsConfigDialog::autoDiscoveryFinished);
        mProgressDialog = new EwsProgressDialog(this, EwsProgressDialog::AutoDiscovery);
        connect(mProgressDialog, &QDialog::rejected, this, &EwsConfigDialog::autoDiscoveryCancelled);
        mAutoDiscoveryJob->start();
        if (!mProgressDialog->exec()) {
            if (KMessageBox::questionYesNo(this,
                                           i18n("Autodiscovery failed. This can be caused by incorrect parameters. Do you still want to save your settings?"),
                                           i18n("Exchange server autodiscovery")) == KMessageBox::Yes) {
                accept();
                return;
            } else {
                return;
            }
        }
    }

    if (mTryConnectNeeded) {
        EwsClient cli;
        cli.setUrl(mUi->kcfg_BaseUrl->text());
        cli.setCredentials(fullUsername(), mUi->passwordEdit->password());
        if (mUi->userAgentGroupBox->isChecked()) {
            cli.setUserAgent(mUi->userAgentEdit->text());
        }
        cli.setEnableNTLMv2(mUi->kcfg_EnableNTLMv2->isChecked());
        mTryConnectJob = new EwsGetFolderRequest(cli, this);
        mTryConnectJob->setFolderShape(EwsFolderShape(EwsShapeIdOnly));
        mTryConnectJob->setFolderIds(EwsId::List() << EwsId(EwsDIdInbox));
        connect(mTryConnectJob, &EwsGetFolderRequest::result, this, &EwsConfigDialog::tryConnectFinished);
        mProgressDialog = new EwsProgressDialog(this, EwsProgressDialog::TryConnect);
        connect(mProgressDialog, &QDialog::rejected, this, &EwsConfigDialog::tryConnectCancelled);
        mTryConnectJob->start();
        if (!mProgressDialog->exec()) {
            if (KMessageBox::questionYesNo(this,
                                           i18n("Connecting to Exchange failed. This can be caused by incorrect parameters. Do you still want to save your settings?"),
                                           i18n("Exchange server connection")) == KMessageBox::Yes) {
                accept();
                return;
            } else {
                return;
            }
        } else {
            accept();
        }
    }

    if (!mTryConnectNeeded && !mAutoDiscoveryNeeded) {
        accept();
    }
}

void EwsConfigDialog::enableTryConnect()
{
    mTryConnectNeeded = true;
    bool baseUrlEmpty = mUi->kcfg_BaseUrl->text().isEmpty();

    /* Enable the OK button when at least the e-mail and username fields are set. Additionally if
     * autodiscovery is disabled, enable the OK button only if the base URL is set. */
    bool okEnabled = !mUi->kcfg_Username->text().isEmpty() && !mUi->kcfg_Email->text().isEmpty();
    if (!mUi->kcfg_AutoDiscovery->isChecked() && baseUrlEmpty) {
        okEnabled = false;
    }
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
    mUi->tryConnectButton->setEnabled(!baseUrlEmpty);
}

void EwsConfigDialog::tryConnect()
{
    EwsClient cli;
    cli.setUrl(mUi->kcfg_BaseUrl->text());
    cli.setCredentials(fullUsername(), mUi->passwordEdit->password());
    if (mUi->userAgentGroupBox->isChecked()) {
        cli.setUserAgent(mUi->userAgentEdit->text());
    }
    cli.setEnableNTLMv2(mUi->kcfg_EnableNTLMv2->isChecked());
    mTryConnectJob = new EwsGetFolderRequest(cli, this);
    mTryConnectJob->setFolderShape(EwsFolderShape(EwsShapeIdOnly));
    mTryConnectJob->setFolderIds(EwsId::List() << EwsId(EwsDIdInbox));
    mProgressDialog = new EwsProgressDialog(this, EwsProgressDialog::TryConnect);
    connect(mProgressDialog, &QDialog::rejected, this, &EwsConfigDialog::tryConnectCancelled);
    mProgressDialog->show();
    if (!mTryConnectJob->exec()) {
        mUi->serverStatusText->setText(i18nc("Exchange server status", "Failed"));
        mUi->serverVersionText->setText(i18nc("Exchange server version", "Unknown"));
        KMessageBox::error(this, mTryConnectJob->errorText(), i18n("Connection failed"));
    } else {
        mUi->serverStatusText->setText(i18nc("Exchange server status", "OK"));
        mUi->serverVersionText->setText(mTryConnectJob->serverVersion().toString());
    }
    mProgressDialog->hide();
}

void EwsConfigDialog::userAgentChanged(int)
{
    QString data = mUi->userAgentCombo->currentData().toString();
    mUi->userAgentEdit->setEnabled(data.isEmpty());
    if (!data.isEmpty()) {
        mUi->userAgentEdit->setText(data);
    }
}
