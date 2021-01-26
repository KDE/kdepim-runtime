/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "setupwizard.h"

#include <KDAV/DavCollectionsMultiFetchJob>
#include <KLocalizedString>
#include <KPasswordLineEdit>
#include <KService>
#include <KServiceTypeTrader>
#include <QIcon>
#include <QLineEdit>
#include <QTextBrowser>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpressionValidator>
#include <QUrl>

enum GroupwareServers { Citadel, DAVical, eGroupware, OpenGroupware, ScalableOGo, Scalix, Zarafa, Zimbra };

static QString settingsToUrl(const QWizard *wizard, const QString &protocol)
{
    const QString desktopFilePath = wizard->property("providerDesktopFilePath").toString();
    if (desktopFilePath.isEmpty()) {
        return QString();
    }

    KService::Ptr service = KService::serviceByStorageId(desktopFilePath);
    if (!service) {
        return QString();
    }

    const QStringList supportedProtocols = service->property(QStringLiteral("X-DavGroupware-SupportedProtocols")).toStringList();
    if (!supportedProtocols.contains(protocol)) {
        return QString();
    }

    QString pathPattern;

    const QString pathPropertyName(QStringLiteral("X-DavGroupware-") + protocol + QStringLiteral("Path"));
    if (service->property(pathPropertyName).isNull()) {
        return QString();
    }

    pathPattern.append(service->property(pathPropertyName).toString() + QLatin1Char('/'));

    QString username = wizard->field(QStringLiteral("credentialsUserName")).toString();
    QString localPart(username);
    localPart.remove(QRegularExpression(QStringLiteral("@.*$")));
    pathPattern.replace(QLatin1String("$user$"), username);
    pathPattern.replace(QLatin1String("$localpart$"), localPart);
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

        const QString host = wizard->field(QStringLiteral("connectionHost")).toString();
        if (host.isEmpty()) {
            return QString();
        }
        const QStringList hostParts = host.split(QLatin1Char(':'));
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
    setWindowTitle(i18nc("@title:window", "DAV groupware configuration wizard"));
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
            url.protocol = KDAV::CalDav;
        } else if (protocol == QLatin1String("CardDav")) {
            url.protocol = KDAV::CardDav;
        } else if (protocol == QLatin1String("GroupDav")) {
            url.protocol = KDAV::GroupDav;
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

    auto layout = new QFormLayout(this);

    mUserName = new QLineEdit;
    layout->addRow(i18n("User:"), mUserName);
    registerField(QStringLiteral("credentialsUserName*"), mUserName);

    mPassword = new KPasswordLineEdit;
    layout->addRow(i18n("Password:"), mPassword);
    registerField(QStringLiteral("credentialsPassword*"), mPassword, "password", SIGNAL(passwordChanged(QString)));
}

int CredentialsPage::nextId() const
{
    QString userName = field(QStringLiteral("credentialsUserName")).toString();
    if (userName.endsWith(QLatin1String("@yahoo.com"))) {
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

    auto layout = new QVBoxLayout(this);

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
    mLabel->setText(
        i18n("Based on the email address you used as a login, this wizard\n"
             "can configure automatically an account for %1 services.\n"
             "Do you wish to do so?",
             wizard()->property("predefinedProviderName").toString()));

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

bool compareServiceOffers(const QPair<QString, QString> &off1, const QPair<QString, QString> &off2)
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
    QList<QPair<QString, QString>> offers;
    offers.reserve(providers.count());
    for (const KService::Ptr &provider : providers) {
        offers.append(QPair<QString, QString>(provider->name(), provider->entryPath()));
    }
    std::sort(offers.begin(), offers.end(), compareServiceOffers);
    QListIterator<QPair<QString, QString>> it(offers);
    while (it.hasNext()) {
        QPair<QString, QString> p = it.next();
        mProvidersCombo->addItem(p.first, p.second);
    }
    registerField(QStringLiteral("provider"), mProvidersCombo, "currentText");

    auto layout = new QVBoxLayout(this);

    mServerGroup = new QButtonGroup(this);
    mServerGroup->setExclusive(true);

    auto hLayout = new QHBoxLayout;
    QRadioButton *button = new QRadioButton(i18n("Use one of those servers:"));
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
    : QWizardPage(parent)
    , mPreviewLayout(nullptr)
    , mCalDavUrlPreview(nullptr)
    , mCardDavUrlPreview(nullptr)
    , mGroupDavUrlPreview(nullptr)
{
    setTitle(i18n("Connection"));
    setSubTitle(i18n("Enter the connection information for the groupware server"));

    mLayout = new QFormLayout(this);
    const QRegularExpression hostnameRegexp(QStringLiteral("^[a-z0-9][.a-z0-9-]*[a-z0-9](?::[0-9]+)?$"));
    mHost = new QLineEdit;
    registerField(QStringLiteral("connectionHost*"), mHost);
    mHost->setValidator(new QRegularExpressionValidator(hostnameRegexp, this));
    mLayout->addRow(i18n("Host"), mHost);

    mPath = new QLineEdit;
    mLayout->addRow(i18n("Installation path"), mPath);
    registerField(QStringLiteral("installationPath"), mPath);

    mUseSecureConnection = new QCheckBox(i18n("Use secure connection"));
    mUseSecureConnection->setChecked(true);
    registerField(QStringLiteral("connectionUseSecureConnection"), mUseSecureConnection);
    mLayout->addRow(QString(), mUseSecureConnection);

    connect(mHost, &QLineEdit::textChanged, this, &ConnectionPage::urlElementChanged);
    connect(mPath, &QLineEdit::textChanged, this, &ConnectionPage::urlElementChanged);
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

    if (supportedProtocols.contains(QLatin1String("CalDav"))) {
        mCalDavUrlLabel = new QLabel(i18n("Final URL (CalDav)"));
        mCalDavUrlPreview = new QLabel;
        mPreviewLayout->addRow(mCalDavUrlLabel, mCalDavUrlPreview);
    }
    if (supportedProtocols.contains(QLatin1String("CardDav"))) {
        mCardDavUrlLabel = new QLabel(i18n("Final URL (CardDav)"));
        mCardDavUrlPreview = new QLabel;
        mPreviewLayout->addRow(mCardDavUrlLabel, mCardDavUrlPreview);
    }
    if (supportedProtocols.contains(QLatin1String("GroupDav"))) {
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
        mCalDavUrlPreview = nullptr;
    }

    if (mCardDavUrlPreview) {
        delete mCardDavUrlLabel;
        delete mCardDavUrlPreview;
        mCardDavUrlPreview = nullptr;
    }

    if (mGroupDavUrlPreview) {
        delete mGroupDavUrlLabel;
        delete mGroupDavUrlPreview;
        mGroupDavUrlPreview = nullptr;
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

    auto layout = new QVBoxLayout(this);

    QPushButton *button = new QPushButton(i18n("Test Connection"));
    layout->addWidget(button);

    mStatusLabel = new QTextBrowser;
    layout->addWidget(mStatusLabel);

    connect(button, &QRadioButton::clicked, this, &CheckPage::checkConnection);
}

void CheckPage::checkConnection()
{
    mStatusLabel->clear();

    KDAV::DavUrl::List davUrls;

    // convert list of SetupWizard::Url to list of KDAV::DavUrl
    const SetupWizard::Url::List urls = static_cast<SetupWizard *>(wizard())->urls();
    for (const SetupWizard::Url &url : urls) {
        KDAV::DavUrl davUrl;
        davUrl.setProtocol(url.protocol);

        QUrl serverUrl(url.url);
        serverUrl.setUserName(wizard()->field(QStringLiteral("credentialsUserName")).toString());
        serverUrl.setPassword(wizard()->field(QStringLiteral("credentialsPassword")).toString());
        davUrl.setUrl(serverUrl);

        davUrls << davUrl;
    }

    // start the dav collections fetch job to test connectivity
    auto job = new KDAV::DavCollectionsMultiFetchJob(davUrls, this);
    connect(job, &KDAV::DavCollectionsMultiFetchJob::result, this, &CheckPage::onFetchDone);
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
