/*
    SPDX-FileCopyrightText: 2009 Grégory Oestreicher <greg@kamago.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configwidget.h"
#include "searchdialog.h"
#include "setupwizard.h"
#include "urlconfigurationdialog.h"
#include "utils.h"

#include <KDAV/ProtocolInfo>

#include <KConfigDialogManager>
#include <KLocalizedString>
#include <KMessageBox>

#include <KWindowConfig>
#include <QDialogButtonBox>
#include <QPointer>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QVBoxLayout>
#include <QWindow>

ConfigWidget::ConfigWidget(Settings &settings, const QString &identifier, QWidget *parent)
    : QWidget(parent)
    , mSettings(settings)
    , mIdentifier(identifier)
    , mModel(new QStandardItemModel(this))
{
    setWindowIcon(QIcon::fromTheme(QStringLiteral("folder-remote")));
    setWindowTitle(i18nc("@title:window", "DAV Resource Configuration"));
    auto mainLayout = new QVBoxLayout(this);
    auto mainWidget = new QWidget(this);
    mainLayout->addWidget(mainWidget);
    mUi.setupUi(mainWidget);
    mSettings.setResourceIdentifier(identifier);

    const QStringList headers = {i18n("Protocol"), i18n("URL")};
    mModel->setHorizontalHeaderLabels(headers);

    mUi.configuredUrls->setModel(mModel);
    mUi.configuredUrls->setRootIsDecorated(false);

    const KDAV::DavUrl::List lstUrls = mSettings.configuredDavUrls();
    for (const KDAV::DavUrl &url : lstUrls) {
        QUrl displayUrl = url.url();
        displayUrl.setUserInfo(QString());
        addModelRow(Utils::translatedProtocolName(url.protocol()), displayUrl.toDisplayString());
    }

    mUi.syncRangeStartType->addItem(i18n("Days"), QVariant(QLatin1StringView("D")));
    mUi.syncRangeStartType->addItem(i18n("Months"), QVariant(QLatin1StringView("M")));
    mUi.syncRangeStartType->addItem(i18n("Years"), QVariant(QLatin1StringView("Y")));

    mManager = new KConfigDialogManager(this, &mSettings);

    connect(mUi.kcfg_displayName, &QLineEdit::textChanged, this, &ConfigWidget::checkUserInput);
    connect(mUi.configuredUrls->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ConfigWidget::checkConfiguredUrlsButtonsState);
    connect(mUi.configuredUrls, &QAbstractItemView::doubleClicked, this, &ConfigWidget::onEditButtonClicked);

    connect(mUi.syncRangeStartType, &QComboBox::currentIndexChanged, this, &ConfigWidget::onSyncRangeStartTypeChanged);
    connect(mUi.addButton, &QPushButton::clicked, this, &ConfigWidget::onAddButtonClicked);
    connect(mUi.searchButton, &QPushButton::clicked, this, &ConfigWidget::onSearchButtonClicked);
    connect(mUi.removeButton, &QPushButton::clicked, this, &ConfigWidget::onRemoveButtonClicked);
    connect(mUi.editButton, &QPushButton::clicked, this, &ConfigWidget::onEditButtonClicked);
}

ConfigWidget::~ConfigWidget() = default;

void ConfigWidget::loadSettings()
{
    const int typeIndex = mUi.syncRangeStartType->findData(QVariant(mSettings.syncRangeStartType()));
    mUi.syncRangeStartType->setCurrentIndex(typeIndex);

    checkUserInput();
    mManager->updateWidgets();

    if (mSettings.defaultUsername().isEmpty()) {
        auto wizard = new SetupWizard(this);
        wizard->setAttribute(Qt::WA_DeleteOnClose);

        connect(wizard, &QWizard::finished, this, [this, wizard] {
            const SetupWizard::Url::List urls = wizard->urls();
            for (const SetupWizard::Url &url : urls) {
                auto urlConfig = new Settings::UrlConfiguration();

                urlConfig->mUrl = url.url;
                urlConfig->mProtocol = url.protocol;
                urlConfig->mUser = url.userName;
                urlConfig->mPassword = wizard->field(QStringLiteral("credentialsPassword")).toString();

                mSettings.newUrlConfiguration(urlConfig);

                QUrl displayUrl(url.url);
                displayUrl.setUserInfo(QString());
                addModelRow(Utils::translatedProtocolName(url.protocol), displayUrl.toDisplayString());
            }

            const QString defaultUser = wizard->field(QStringLiteral("credentialsUserName")).toString();

            if (!urls.isEmpty()) {
                mSettings.setDisplayName(wizard->displayName());
            } else {
                mSettings.setDisplayName(defaultUser);
            }

            if (!defaultUser.isEmpty()) {
                const auto password = wizard->field(QStringLiteral("credentialsPassword")).toString();
                mSettings.setDefaultUsername(defaultUser);
                mSettings.setDefaultPassword(password);
                setPassword(password);
            }

            mManager->updateWidgets();
        });
        wizard->show();
    }
}

void ConfigWidget::setPassword(const QString &password)
{
    mUi.password->setPassword(password);
}

void ConfigWidget::onSyncRangeStartTypeChanged()
{
    mSettings.setSyncRangeStartType(mUi.syncRangeStartType->currentData().toString());
}

void ConfigWidget::checkUserInput()
{
    checkConfiguredUrlsButtonsState();

    if (!mUi.kcfg_displayName->text().trimmed().isEmpty() && !(mModel->invisibleRootItem()->rowCount() == 0)) {
        Q_EMIT okEnabled(true);
    } else {
        Q_EMIT okEnabled(false);
    }
}

void ConfigWidget::onAddButtonClicked()
{
    QPointer<UrlConfigurationDialog> dlg = new UrlConfigurationDialog(this);
    dlg->setDefaultUsername(mUi.kcfg_defaultUsername->text());
    dlg->setDefaultPassword(mUi.password->password());
    const int result = dlg->exec();

    if (result == QDialog::Accepted && !dlg.isNull()) {
        if (mSettings.urlConfiguration(KDAV::Protocol(dlg->protocol()), dlg->remoteUrl())) {
            KMessageBox::error(this,
                               i18n("Another configuration entry already uses the same URL/protocol couple.\n"
                                    "Please use a different URL"));
        } else {
            auto urlConfig = new Settings::UrlConfiguration();

            urlConfig->mUrl = dlg->remoteUrl();
            if (dlg->useDefaultCredentials()) {
                urlConfig->mUser = QStringLiteral("$default$");
            } else {
                urlConfig->mUser = dlg->username();
                urlConfig->mPassword = dlg->password();
            }
            urlConfig->mProtocol = dlg->protocol();

            mSettings.newUrlConfiguration(urlConfig);

            const QString protocolName = Utils::translatedProtocolName(dlg->protocol());

            addModelRow(protocolName, dlg->remoteUrl());
            mAddedUrls << QPair<QString, KDAV::Protocol>(dlg->remoteUrl(), KDAV::Protocol(dlg->protocol()));
            checkUserInput();
        }
    }

    delete dlg;
}

void ConfigWidget::onSearchButtonClicked()
{
    QPointer<SearchDialog> dlg = new SearchDialog(this);
    dlg->setUsername(mUi.kcfg_defaultUsername->text());
    dlg->setPassword(mUi.password->password());
    const int result = dlg->exec();

    if (result == QDialog::Accepted && !dlg.isNull()) {
        const QStringList results = dlg->selection();
        for (const QString &resultStr : results) {
            const QStringList split = resultStr.split(QLatin1Char('|'));
            KDAV::Protocol protocol = KDAV::ProtocolInfo::protocolByName(split.at(0));
            if (!mSettings.urlConfiguration(protocol, split.at(1))) {
                auto urlConfig = new Settings::UrlConfiguration();

                urlConfig->mUrl = split.at(1);
                if (dlg->useDefaultCredentials()) {
                    urlConfig->mUser = QStringLiteral("$default$");
                } else {
                    urlConfig->mUser = dlg->username();
                    urlConfig->mPassword = dlg->password();
                }
                urlConfig->mProtocol = protocol;

                mSettings.newUrlConfiguration(urlConfig);

                addModelRow(Utils::translatedProtocolName(protocol), split.at(1));
                mAddedUrls << QPair<QString, KDAV::Protocol>(split.at(1), protocol);
                checkUserInput();
            }
        }
    }

    delete dlg;
}

void ConfigWidget::onRemoveButtonClicked()
{
    const QModelIndexList indexes = mUi.configuredUrls->selectionModel()->selectedRows();
    if (indexes.isEmpty()) {
        return;
    }

    const QString proto = mModel->index(indexes.at(0).row(), 0).data().toString();
    const QString url = mModel->index(indexes.at(0).row(), 1).data().toString();

    mRemovedUrls << QPair<QString, KDAV::Protocol>(url, Utils::protocolByTranslatedName(proto));
    mModel->removeRow(indexes.at(0).row());

    checkUserInput();
}

void ConfigWidget::onEditButtonClicked()
{
    const QModelIndexList indexes = mUi.configuredUrls->selectionModel()->selectedRows();
    if (indexes.isEmpty()) {
        return;
    }

    const int row = indexes.at(0).row();
    const QString proto = mModel->index(row, 0).data().toString();
    const QString url = mModel->index(row, 1).data().toString();

    Settings::UrlConfiguration *urlConfig = mSettings.urlConfiguration(Utils::protocolByTranslatedName(proto), url);
    if (!urlConfig) {
        return;
    }

    QPointer<UrlConfigurationDialog> dlg = new UrlConfigurationDialog(this);
    dlg->setRemoteUrl(urlConfig->mUrl);
    dlg->setProtocol(KDAV::Protocol(urlConfig->mProtocol));

    if (urlConfig->mUser == QLatin1StringView("$default$")) {
        dlg->setUseDefaultCredentials(true);
    } else {
        dlg->setUseDefaultCredentials(false);
        dlg->setUsername(urlConfig->mUser);
        dlg->setPassword(urlConfig->mPassword);
    }
    dlg->setDefaultUsername(mUi.kcfg_defaultUsername->text());
    dlg->setDefaultPassword(mUi.password->password());

    const int result = dlg->exec();

    if (result == QDialog::Accepted && !dlg.isNull()) {
        mSettings.removeUrlConfiguration(Utils::protocolByTranslatedName(proto), url);
        auto urlConfigAccepted = new Settings::UrlConfiguration();
        urlConfigAccepted->mUrl = dlg->remoteUrl();
        if (dlg->useDefaultCredentials()) {
            urlConfigAccepted->mUser = QStringLiteral("$default$");
        } else {
            urlConfigAccepted->mUser = dlg->username();
            urlConfigAccepted->mPassword = dlg->password();
        }
        urlConfigAccepted->mProtocol = dlg->protocol();
        mSettings.newUrlConfiguration(urlConfigAccepted);

        mModel->removeRow(row);
        insertModelRow(row, Utils::translatedProtocolName(dlg->protocol()), dlg->remoteUrl());
    }
    delete dlg;
}

void ConfigWidget::saveSettings() const
{
    using UrlPair = QPair<QString, KDAV::Protocol>;
    for (const UrlPair &url : std::as_const(mRemovedUrls)) {
        mSettings.removeUrlConfiguration(url.second, url.first);
    }

    mManager->updateSettings();
    mSettings.setDefaultPassword(mUi.password->password());
    mSettings.setSettingsVersion(3);
    mSettings.save();
}

void ConfigWidget::checkConfiguredUrlsButtonsState()
{
    const bool enabled = mUi.configuredUrls->selectionModel()->hasSelection();

    mUi.removeButton->setEnabled(enabled);
    mUi.editButton->setEnabled(enabled);
}

void ConfigWidget::addModelRow(const QString &protocol, const QString &url)
{
    insertModelRow(-1, protocol, url);
}

void ConfigWidget::insertModelRow(int index, const QString &protocol, const QString &url)
{
    QStandardItem *rootItem = mModel->invisibleRootItem();
    QList<QStandardItem *> items;

    auto protocolStandardItem = new QStandardItem(protocol);
    protocolStandardItem->setEditable(false);
    items << protocolStandardItem;

    auto urlStandardItem = new QStandardItem(url);
    urlStandardItem->setEditable(false);
    items << urlStandardItem;

    if (index == -1) {
        rootItem->appendRow(items);
    } else {
        rootItem->insertRow(index, items);
    }
}

#include "moc_configwidget.cpp"
