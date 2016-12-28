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

#include "davcollectionsmultifetchjob.h"

#include <qicon.h>
#include <KLocalizedString>
#include <klineedit.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <QTextBrowser>

#include <QUrl>
#include <QButtonGroup>
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExpValidator>

enum GroupwareServers {
    Citadel,
    DAVical,
    eGroupware,
    OpenGroupware,
    ScalableOGo,
    Scalix,
    Zarafa,
    Zimbra
};

static QString settingsToUrl(const QWizard *wizard, const QString &protocol)
{
    QString desktopFilePath = wizard->property("providerDesktopFilePath").toString();
    if (desktopFilePath.isEmpty()) {
        return QString();
    }

    KService::Ptr service = KService::serviceByStorageId(desktopFilePath);
    if (!service) {
        return QString();
    }

    QStringList supportedProtocols = service->property(QStringLiteral("X-DavGroupware-SupportedProtocols")).toStringList();
    if (!supportedProtocols.contains(protocol)) {
        return QString();
    }

    QString pathPattern;

    QString pathPropertyName(QStringLiteral("X-DavGroupware-") + protocol + QStringLiteral("Path"));
    if (service->property(pathPropertyName).isNull()) {
        return QString();
    }

    pathPattern.append(service->property(pathPropertyName).toString() + QLatin1Char('/'));

    QString username = wizard->field(QStringLiteral("credentialsUserName")).toString();
    QString localPart(username);
    localPart.remove(QRegExp(QLatin1String("@.*$")));
    pathPattern.replace(QStringLiteral("$user$"), username);
    pathPattern.replace(QStringLiteral("$localpart$"), localPart);
    QString providerName;
    if (!service->property(QStringLiteral("X-DavGroupware-Provider")).isNull()) {
        providerName = service->property(QStringLiteral("X-DavGroupware-Provider")).toString();
    }
    QString localPath = wizard->field(QStringLiteral("installationPath")).toString();
    if (!localPath.isEmpty()) {
        if (providerName == QLatin1String("davical")) {
            if (!localPath.endsWith(QLatin1Char('/'))) {
                pathPattern.append(localPath + QLatin1Char('/'));
            } else {
                pathPattern.append(localPath);
            }
        } else {
            if (!localPath.startsWith(QLatin1Char('/'))) {
                pathPattern.prepend(QLatin1Char('/') + localPath);
            } else {
                pathPattern.prepend(localPath);
            }
        }
    }
    QUrl url;

    if (!wizard->property("usePredefinedProvider").isNull()) {
        if (service->property(QStringLiteral("X-DavGroupware-ProviderUsesSSL")).toBool()) {
            url.setScheme(QStringLiteral("https"));
        } else {
            url.setScheme(QStringLiteral("http"));
        }

        QString hostPropertyName(QStringLiteral("X-DavGroupware-") + protocol + QStringLiteral("Host"));
        if (service->property(hostPropertyName).isNull()) {
            return QString();
        }

        url.setHost(service->property(hostPropertyName).toString());
        url.setPath(pathPattern);
    } else {
        if (wizard->field(QStringLiteral("connectionUseSecureConnection")).toBool()) {
            url.setScheme(QStringLiteral("https"));
        } else {
            url.setScheme(QStringLiteral("http"));
        }

        QString host = wizard->field(QStringLiteral("connectionHost")).toString();
        if (host.isEmpty()) {
            return QString();
        }
        QStringList hostParts = host.split(QLatin1Char(':'));
        url.setHost(hostParts.at(0));
        url.setPath(pathPattern);

        if (hostParts.size() == 2) {
            int port = hostParts.at(1).toInt();
            if (port) {
                url.setPort(port);
            }
        }
    }
    return url.toString();
}

/*
 * SetupWizard
 */

SetupWizard::SetupWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(i18n("DAV groupware configuration wizard"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("folder-remote")));
    setPage(W_CredentialsPage, new CredentialsPage);
    setPage(W_PredefinedProviderPage, new PredefinedProviderPage);
    setPage(W_ServerTypePage, new ServerTypePage);
    setPage(W_ConnectionPage, new ConnectionPage);
    setPage(W_CheckPage, new CheckPage);
}

QString SetupWizard::displayName() const
{
    QString desktopFilePath = property("providerDesktopFilePath").toString();
    if (desktopFilePath.isEmpty()) {
        return QString();
    }

    KService::Ptr service = KService::serviceByStorageId(desktopFilePath);
    if (!service) {
        return QString();
    }

    return service->name();
}

SetupWizard::Url::List SetupWizard::urls() const
{
    Url::List urls;

    QString desktopFilePath = property("providerDesktopFilePath").toString();
    if (desktopFilePath.isEmpty()) {
        return urls;
    }

    KService::Ptr service = KService::serviceByStorageId(desktopFilePath);
    if (!service) {
        return urls;
    }

    const QStringList supportedProtocols = service->property(QStringLiteral("X-DavGroupware-SupportedProtocols")).toStringList();
    for (const QString &protocol : supportedProtocols) {
        Url url;

        if (protocol == QLatin1String("CalDav")) {
            url.protocol = DavUtils::CalDav;
        } else if (protocol == QLatin1String("CardDav")) {
            url.protocol = DavUtils::CardDav;
        } else if (protocol == QLatin1String("GroupDav")) {
            url.protocol = DavUtils::GroupDav;
        } else {
            return urls;
        }

        QString urlStr = settingsToUrl(this, protocol);

        if (!urlStr.isEmpty()) {
            url.url = urlStr;
            url.userName = QStringLiteral("$default$");
            urls << url;
        }
    }

    return urls;
}

/*
 * CredentialsPage
 */

CredentialsPage::CredentialsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18n("Login Credentials"));
    setSubTitle(i18n("Enter your credentials to login to the groupware server"));

    QFormLayout *layout = new QFormLayout(this);

    mUserName = new KLineEdit;
    layout->addRow(i18n("User"), mUserName);
    registerField(QStringLiteral("credentialsUserName*"), mUserName);

    mPassword = new KLineEdit;
    mPassword->setPasswordMode(true);
    layout->addRow(i18n("Password"), mPassword);
    registerField(QStringLiteral("credentialsPassword*"), mPassword);
}

int CredentialsPage::nextId() const
{
    QString userName = field(QStringLiteral("credentialsUserName")).toString();
    if (userName.endsWith(QStringLiteral("@yahoo.com"))) {
        KService::List offers;
        offers = KServiceTypeTrader::self()->query(QStringLiteral("DavGroupwareProvider"), QStringLiteral("Name == 'Yahoo!'"));
        if (offers.isEmpty()) {
            return SetupWizard::W_ServerTypePage;
        }

        wizard()->setProperty("usePredefinedProvider", true);
        wizard()->setProperty("predefinedProviderName", offers.at(0)->name());
        wizard()->setProperty("providerDesktopFilePath", offers.at(0)->entryPath());
        return SetupWizard::W_PredefinedProviderPage;
    } else {
        return SetupWizard::W_ServerTypePage;
    }
}

/*
 * PredefinedProviderPage
 */

PredefinedProviderPage::PredefinedProviderPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18n("Predefined provider found"));
    setSubTitle(i18n("Select if you want to use the auto-detected provider"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    mLabel = new QLabel;
    layout->addWidget(mLabel);

    mProviderGroup = new QButtonGroup(this);
    mProviderGroup->setExclusive(true);

    mUseProvider = new QRadioButton;
    mProviderGroup->addButton(mUseProvider);
    mUseProvider->setChecked(true);
    layout->addWidget(mUseProvider);

    mDontUseProvider = new QRadioButton(i18n("No, choose another server"));
    mProviderGroup->addButton(mDontUseProvider);
    layout->addWidget(mDontUseProvider);
}

void PredefinedProviderPage::initializePage()
{
    mLabel->setText(i18n("Based on the email address you used as a login, this wizard\n"
                         "can configure automatically an account for %1 services.\n"
                         "Do you wish to do so?", wizard()->property("predefinedProviderName").toString()));

    mUseProvider->setText(i18n("Yes, use %1 as provider", wizard()->property("predefinedProviderName").toString()));
}

int PredefinedProviderPage::nextId() const
{
    if (mUseProvider->isChecked()) {
        return SetupWizard::W_CheckPage;
    } else {
        wizard()->setProperty("usePredefinedProvider", QVariant());
        wizard()->setProperty("providerDesktopFilePath", QVariant());
        return SetupWizard::W_ServerTypePage;
    }
}

/*
 * ServerTypePage
 */

bool compareServiceOffers(QPair<QString, QString> off1, QPair<QString, QString> off2)
{
    return off1.first.toLower() < off2.first.toLower();
}

ServerTypePage::ServerTypePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18n("Groupware Server"));
    setSubTitle(i18n("Select the groupware server the resource shall be configured for"));

    mProvidersCombo = new QComboBox(this);
    mProvidersCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    KServiceTypeTrader *trader = KServiceTypeTrader::self();
    const KService::List providers = trader->query(QStringLiteral("DavGroupwareProvider"));
    QList< QPair<QString, QString> > offers;
    offers.reserve(providers.count());
    for (const KService::Ptr &provider : providers) {
        offers.append(QPair<QString, QString>(provider->name(), provider->entryPath()));
    }
    std::sort(offers.begin(), offers.end(), compareServiceOffers);
    QListIterator< QPair<QString, QString> > it(offers);
    while (it.hasNext()) {
        QPair<QString, QString> p = it.next();
        mProvidersCombo->addItem(p.first, p.second);
    }
    registerField(QStringLiteral("provider"), mProvidersCombo, "currentText");

    QVBoxLayout *layout = new QVBoxLayout(this);

    mServerGroup = new QButtonGroup(this);
    mServerGroup->setExclusive(true);

    QRadioButton *button;

    QHBoxLayout *hLayout = new QHBoxLayout;
    button = new QRadioButton(i18n("Use one of those servers:"));
    registerField(QStringLiteral("templateConfiguration"), button);
    mServerGroup->addButton(button);
    mServerGroup->setId(button, 0);
    button->setChecked(true);
    hLayout->addWidget(button);
    hLayout->addWidget(mProvidersCombo);
    layout->addLayout(hLayout);

    button = new QRadioButton(i18n("Configure the resource manually"));
    connect(button, &QRadioButton::toggled, this, &ServerTypePage::manualConfigToggled);
    registerField(QStringLiteral("manualConfiguration"), button);
    mServerGroup->addButton(button);
    mServerGroup->setId(button, 1);
    layout->addWidget(button);

    layout->addStretch(1);
}

void ServerTypePage::manualConfigToggled(bool state)
{
    setFinalPage(state);
    wizard()->button(QWizard::NextButton)->setEnabled(!state);
}

bool ServerTypePage::validatePage()
{
    QVariant desktopFilePath = mProvidersCombo->itemData(mProvidersCombo->currentIndex());
    if (desktopFilePath.isNull()) {
        return false;
    } else {
        wizard()->setProperty("providerDesktopFilePath", desktopFilePath);
        return true;
    }
}

/*
 * ConnectionPage
 */

ConnectionPage::ConnectionPage(QWidget *parent)
    : QWizardPage(parent), mPreviewLayout(Q_NULLPTR), mCalDavUrlPreview(Q_NULLPTR), mCardDavUrlPreview(Q_NULLPTR), mGroupDavUrlPreview(Q_NULLPTR)
{
    setTitle(i18n("Connection"));
    setSubTitle(i18n("Enter the connection information for the groupware server"));

    mLayout = new QFormLayout(this);
    QRegExp hostnameRegexp(QStringLiteral("^[a-z0-9][.a-z0-9-]*[a-z0-9](?::[0-9]+)?$"));
    mHost = new KLineEdit;
    registerField(QStringLiteral("connectionHost*"), mHost);
    mHost->setValidator(new QRegExpValidator(hostnameRegexp, this));
    mLayout->addRow(i18n("Host"), mHost);

    mPath = new KLineEdit;
    mLayout->addRow(i18n("Installation path"), mPath);
    registerField(QStringLiteral("installationPath"), mPath);

    mUseSecureConnection = new QCheckBox(i18n("Use secure connection"));
    mUseSecureConnection->setChecked(true);
    registerField(QStringLiteral("connectionUseSecureConnection"), mUseSecureConnection);
    mLayout->addRow(QString(), mUseSecureConnection);

    connect(mHost, &KLineEdit::textChanged, this, &ConnectionPage::urlElementChanged);
    connect(mPath, &KLineEdit::textChanged, this, &ConnectionPage::urlElementChanged);
    connect(mUseSecureConnection, &QCheckBox::toggled, this, &ConnectionPage::urlElementChanged);
}

void ConnectionPage::initializePage()
{
    KService::Ptr service = KService::serviceByStorageId(wizard()->property("providerDesktopFilePath").toString());
    if (!service) {
        return;
    }

    QString providerInstallationPath = service->property(QStringLiteral("X-DavGroupware-InstallationPath")).toString();
    if (!providerInstallationPath.isEmpty()) {
        mPath->setText(providerInstallationPath);
    }

    QStringList supportedProtocols = service->property(QStringLiteral("X-DavGroupware-SupportedProtocols")).toStringList();

    mPreviewLayout = new QFormLayout;
    mLayout->addRow(mPreviewLayout);

    if (supportedProtocols.contains(QStringLiteral("CalDav"))) {
        mCalDavUrlLabel = new QLabel(i18n("Final URL (CalDav)"));
        mCalDavUrlPreview = new QLabel;
        mPreviewLayout->addRow(mCalDavUrlLabel, mCalDavUrlPreview);
    }
    if (supportedProtocols.contains(QStringLiteral("CardDav"))) {
        mCardDavUrlLabel = new QLabel(i18n("Final URL (CardDav)"));
        mCardDavUrlPreview = new QLabel;
        mPreviewLayout->addRow(mCardDavUrlLabel, mCardDavUrlPreview);
    }
    if (supportedProtocols.contains(QStringLiteral("GroupDav"))) {
        mGroupDavUrlLabel = new QLabel(i18n("Final URL (GroupDav)"));
        mGroupDavUrlPreview = new QLabel;
        mPreviewLayout->addRow(mGroupDavUrlLabel, mGroupDavUrlPreview);
    }
}

void ConnectionPage::cleanupPage()
{
    delete mPreviewLayout;

    if (mCalDavUrlPreview) {
        delete mCalDavUrlLabel;
        delete mCalDavUrlPreview;
        mCalDavUrlPreview = Q_NULLPTR;
    }

    if (mCardDavUrlPreview) {
        delete mCardDavUrlLabel;
        delete mCardDavUrlPreview;
        mCardDavUrlPreview = Q_NULLPTR;
    }

    if (mGroupDavUrlPreview) {
        delete mGroupDavUrlLabel;
        delete mGroupDavUrlPreview;
        mGroupDavUrlPreview = Q_NULLPTR;
    }

    QWizardPage::cleanupPage();
}

void ConnectionPage::urlElementChanged()
{
    if (mHost->text().isEmpty()) {
        if (mCalDavUrlPreview) {
            mCalDavUrlPreview->setText(QStringLiteral("-"));
        }
        if (mCardDavUrlPreview) {
            mCardDavUrlPreview->setText(QStringLiteral("-"));
        }
        if (mGroupDavUrlPreview) {
            mGroupDavUrlPreview->setText(QStringLiteral("-"));
        }
    } else {
        if (mCalDavUrlPreview) {
            mCalDavUrlPreview->setText(settingsToUrl(this->wizard(), QStringLiteral("CalDav")));
        }
        if (mCardDavUrlPreview) {
            mCardDavUrlPreview->setText(settingsToUrl(this->wizard(), QStringLiteral("CardDav")));
        }
        if (mGroupDavUrlPreview) {
            mGroupDavUrlPreview->setText(settingsToUrl(this->wizard(), QStringLiteral("GroupDav")));
        }
    }
}

/*
 * CheckPage
 */

CheckPage::CheckPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18n("Test Connection"));
    setSubTitle(i18n("You can test now whether the groupware server can be accessed with the current configuration"));
    setFinalPage(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *button = new QPushButton(i18n("Test Connection"));
    layout->addWidget(button);

    mStatusLabel = new QTextBrowser;
    layout->addWidget(mStatusLabel);

    connect(button, &QRadioButton::clicked, this, &CheckPage::checkConnection);
}

void CheckPage::checkConnection()
{
    mStatusLabel->clear();

    DavUtils::DavUrl::List davUrls;

    // convert list of SetupWizard::Url to list of DavUtils::DavUrl
    const SetupWizard::Url::List urls = static_cast<SetupWizard *>(wizard())->urls();
    for (const SetupWizard::Url &url : urls) {
        DavUtils::DavUrl davUrl;
        davUrl.setProtocol(url.protocol);

        QUrl serverUrl(url.url);
        serverUrl.setUserName(wizard()->field(QStringLiteral("credentialsUserName")).toString());
        serverUrl.setPassword(wizard()->field(QStringLiteral("credentialsPassword")).toString());
        davUrl.setUrl(serverUrl);

        davUrls << davUrl;
    }

    // start the dav collections fetch job to test connectivity
    DavCollectionsMultiFetchJob *job = new DavCollectionsMultiFetchJob(davUrls, this);
    connect(job, &DavCollectionsMultiFetchJob::result, this, &CheckPage::onFetchDone);
    job->start();
}

void CheckPage::onFetchDone(KJob *job)
{
    QString msg;
    QPixmap icon;

    if (job->error()) {
        msg = i18n("An error occurred: %1", job->errorText());
        icon = QIcon::fromTheme(QStringLiteral("dialog-close")).pixmap(16, 16);
    } else {
        msg = i18n("Connected successfully");
        icon = QIcon::fromTheme(QStringLiteral("dialog-ok-apply")).pixmap(16, 16);
    }

    mStatusLabel->setHtml(QStringLiteral("<html><body><img src=\"icon\"> %1</body></html>").arg(msg));
    mStatusLabel->document()->addResource(QTextDocument::ImageResource, QUrl(QStringLiteral("icon")), QVariant(icon));
}

