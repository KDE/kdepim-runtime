/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

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

#include "configdialog.h"
#include "searchdialog.h"
#include "settings.h"
#include "utils.h"
#include "urlconfigurationdialog.h"

#include <KDAV/ProtocolInfo>

#include <KConfigDialogManager>
#include <KConfigSkeleton>
#include <KLocalizedString>
#include <KMessageBox>

#include <QList>
#include <QPointer>
#include <QStringList>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QDialogButtonBox>

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowIcon(QIcon::fromTheme(QStringLiteral("folder-remote")));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QWidget *mainWidget = new QWidget(this);
    mainLayout->addWidget(mainWidget);
    mUi.setupUi(mainWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::onCancelClicked);
    mainLayout->addWidget(buttonBox);

    mModel = new QStandardItemModel();
    const QStringList headers = { i18n("Protocol"), i18n("URL") };
    mModel->setHorizontalHeaderLabels(headers);

    mUi.configuredUrls->setModel(mModel);
    mUi.configuredUrls->setRootIsDecorated(false);

    const KDAV::DavUrl::List lstUrls = Settings::self()->configuredDavUrls();
    for (const KDAV::DavUrl &url : lstUrls) {
        QUrl displayUrl = url.url();
        displayUrl.setUserInfo(QString());
        addModelRow(Utils::translatedProtocolName(url.protocol()), displayUrl.toDisplayString());
    }

    mUi.syncRangeStartType->addItem(i18n("Days"), QVariant(QLatin1String("D")));
    mUi.syncRangeStartType->addItem(i18n("Months"), QVariant(QLatin1String("M")));
    mUi.syncRangeStartType->addItem(i18n("Years"), QVariant(QLatin1String("Y")));

    mManager = new KConfigDialogManager(this, Settings::self());
    mManager->updateWidgets();

    const int typeIndex = mUi.syncRangeStartType->findData(QVariant(Settings::self()->syncRangeStartType()));
    mUi.syncRangeStartType->setCurrentIndex(typeIndex);

    connect(mUi.kcfg_displayName, &KLineEdit::textChanged, this, &ConfigDialog::checkUserInput);
    connect(mUi.configuredUrls->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ConfigDialog::checkConfiguredUrlsButtonsState);
    connect(mUi.configuredUrls, &QAbstractItemView::doubleClicked, this, &ConfigDialog::onEditButtonClicked);

    connect(mUi.syncRangeStartType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigDialog::onSyncRangeStartTypeChanged);
    connect(mUi.addButton, &QPushButton::clicked, this, &ConfigDialog::onAddButtonClicked);
    connect(mUi.searchButton, &QPushButton::clicked, this, &ConfigDialog::onSearchButtonClicked);
    connect(mUi.removeButton, &QPushButton::clicked, this, &ConfigDialog::onRemoveButtonClicked);
    connect(mUi.editButton, &QPushButton::clicked, this, &ConfigDialog::onEditButtonClicked);

    connect(mOkButton, &QPushButton::clicked, this, &ConfigDialog::onOkClicked);

    checkUserInput();
    readConfig();
}

ConfigDialog::~ConfigDialog()
{
    writeConfig();
}

void ConfigDialog::readConfig()
{
    KConfigGroup grp(KSharedConfig::openConfig(), "ConfigDialog");
    const QSize size = grp.readEntry("Size", QSize(300, 200));
    if (size.isValid()) {
        resize(size);
    }
}

void ConfigDialog::writeConfig()
{
    KConfigGroup grp(KSharedConfig::openConfig(), "ConfigDialog");
    grp.writeEntry("Size", size());
    grp.sync();
}

void ConfigDialog::setPassword(const QString &password)
{
    mUi.password->setPassword(password);
}

void ConfigDialog::onSyncRangeStartTypeChanged()
{
    Settings::self()->setSyncRangeStartType(mUi.syncRangeStartType->currentData().toString());
}

void ConfigDialog::checkUserInput()
{
    checkConfiguredUrlsButtonsState();

    if (!mUi.kcfg_displayName->text().trimmed().isEmpty() && !(mModel->invisibleRootItem()->rowCount() == 0)) {
        mOkButton->setEnabled(true);
    } else {
        mOkButton->setEnabled(false);
    }
}

void ConfigDialog::onAddButtonClicked()
{
    QPointer<UrlConfigurationDialog> dlg = new UrlConfigurationDialog(this);
    dlg->setDefaultUsername(mUi.kcfg_defaultUsername->text());
    dlg->setDefaultPassword(mUi.password->password());
    const int result = dlg->exec();

    if (result == QDialog::Accepted && !dlg.isNull()) {
        if (Settings::self()->urlConfiguration(KDAV::Protocol(dlg->protocol()), dlg->remoteUrl())) {
            KMessageBox::error(this, i18n("Another configuration entry already uses the same URL/protocol couple.\n"
                                          "Please use a different URL"));
        } else {
            Settings::UrlConfiguration *urlConfig = new Settings::UrlConfiguration();

            urlConfig->mUrl = dlg->remoteUrl();
            if (dlg->useDefaultCredentials()) {
                urlConfig->mUser = QStringLiteral("$default$");
            } else {
                urlConfig->mUser = dlg->username();
                urlConfig->mPassword = dlg->password();
            }
            urlConfig->mProtocol = dlg->protocol();

            Settings::self()->newUrlConfiguration(urlConfig);

            const QString protocolName = Utils::translatedProtocolName(dlg->protocol());

            addModelRow(protocolName, dlg->remoteUrl());
            mAddedUrls << QPair<QString, KDAV::Protocol>(dlg->remoteUrl(), KDAV::Protocol(dlg->protocol()));
            checkUserInput();
        }
    }

    delete dlg;
}

void ConfigDialog::onSearchButtonClicked()
{
    QPointer<SearchDialog> dlg = new SearchDialog(this);
    dlg->setUsername(mUi.kcfg_defaultUsername->text());
    dlg->setPassword(mUi.password->password());
    const int result = dlg->exec();

    if (result == QDialog::Accepted && !dlg.isNull()) {
        const QStringList results = dlg->selection();
        for (const QString &result : results) {
            const QStringList split = result.split(QLatin1Char('|'));
            KDAV::Protocol protocol = KDAV::ProtocolInfo::protocolByName(split.at(0));
            if (!Settings::self()->urlConfiguration(protocol, split.at(1))) {
                Settings::UrlConfiguration *urlConfig = new Settings::UrlConfiguration();

                urlConfig->mUrl = split.at(1);
                if (dlg->useDefaultCredentials()) {
                    urlConfig->mUser = QStringLiteral("$default$");
                } else {
                    urlConfig->mUser = dlg->username();
                    urlConfig->mPassword = dlg->password();
                }
                urlConfig->mProtocol = protocol;

                Settings::self()->newUrlConfiguration(urlConfig);

                addModelRow(Utils::translatedProtocolName(protocol), split.at(1));
                mAddedUrls << QPair<QString, KDAV::Protocol>(split.at(1), protocol);
                checkUserInput();
            }
        }
    }

    delete dlg;
}

void ConfigDialog::onRemoveButtonClicked()
{
    const QModelIndexList indexes = mUi.configuredUrls->selectionModel()->selectedRows();
    if (indexes.isEmpty()) {
        return;
    }

    QString proto = mModel->index(indexes.at(0).row(), 0).data().toString();
    QString url = mModel->index(indexes.at(0).row(), 1).data().toString();

    mRemovedUrls << QPair<QString, KDAV::Protocol>(url, Utils::protocolByTranslatedName(proto));
    mModel->removeRow(indexes.at(0).row());

    checkUserInput();
}

void ConfigDialog::onEditButtonClicked()
{
    const QModelIndexList indexes = mUi.configuredUrls->selectionModel()->selectedRows();
    if (indexes.isEmpty()) {
        return;
    }

    const int row = indexes.at(0).row();
    const QString proto = mModel->index(row, 0).data().toString();
    const QString url = mModel->index(row, 1).data().toString();

    Settings::UrlConfiguration *urlConfig = Settings::self()->urlConfiguration(Utils::protocolByTranslatedName(proto), url);
    if (!urlConfig) {
        return;
    }

    QPointer<UrlConfigurationDialog> dlg = new UrlConfigurationDialog(this);
    dlg->setRemoteUrl(urlConfig->mUrl);
    dlg->setProtocol(KDAV::Protocol(urlConfig->mProtocol));

    if (urlConfig->mUser == QLatin1String("$default$")) {
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
        Settings::self()->removeUrlConfiguration(Utils::protocolByTranslatedName(proto), url);
        Settings::UrlConfiguration *urlConfigAccepted = new Settings::UrlConfiguration();
        urlConfigAccepted->mUrl = dlg->remoteUrl();
        if (dlg->useDefaultCredentials()) {
            urlConfigAccepted->mUser = QStringLiteral("$default$");
        } else {
            urlConfigAccepted->mUser = dlg->username();
            urlConfigAccepted->mPassword = dlg->password();
        }
        urlConfigAccepted->mProtocol = dlg->protocol();
        Settings::self()->newUrlConfiguration(urlConfigAccepted);

        mModel->removeRow(row);
        insertModelRow(row, Utils::translatedProtocolName(dlg->protocol()), dlg->remoteUrl());
    }
    delete dlg;
}

void ConfigDialog::onOkClicked()
{
    typedef QPair<QString, KDAV::Protocol> UrlPair;
    for (const UrlPair &url : qAsConst(mRemovedUrls)) {
        Settings::self()->removeUrlConfiguration(url.second, url.first);
    }

    mManager->updateSettings();
    Settings::self()->setDefaultPassword(mUi.password->password());
    accept();
}

void ConfigDialog::onCancelClicked()
{
    mRemovedUrls.clear();

    typedef QPair<QString, KDAV::Protocol> UrlPair;
    for (const UrlPair &url : qAsConst(mAddedUrls)) {
        Settings::self()->removeUrlConfiguration(url.second, url.first);
    }
    reject();
}

void ConfigDialog::checkConfiguredUrlsButtonsState()
{
    const bool enabled = mUi.configuredUrls->selectionModel()->hasSelection();

    mUi.removeButton->setEnabled(enabled);
    mUi.editButton->setEnabled(enabled);
}

void ConfigDialog::addModelRow(const QString &protocol, const QString &url)
{
    insertModelRow(-1, protocol, url);
}

void ConfigDialog::insertModelRow(int index, const QString &protocol, const QString &url)
{
    QStandardItem *rootItem = mModel->invisibleRootItem();
    QList<QStandardItem *> items;

    QStandardItem *protocolStandardItem = new QStandardItem(protocol);
    protocolStandardItem->setEditable(false);
    items << protocolStandardItem;

    QStandardItem *urlStandardItem = new QStandardItem(url);
    urlStandardItem->setEditable(false);
    items << urlStandardItem;

    if (index == -1) {
        rootItem->appendRow(items);
    } else {
        rootItem->insertRow(index, items);
    }
}
