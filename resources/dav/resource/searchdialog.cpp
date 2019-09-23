/*
 *  Copyright (c) 2011 Grégory Oestreicher <greg@kamago.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "searchdialog.h"

#include <KDAV/DavCollectionsFetchJob>
#include <KDAV/DavManager>
#include <KDAV/DavPrincipalSearchJob>
#include <KDAV/ProtocolInfo>
#include <KDAV/Utils>

#include "davresource_debug.h"
#include <QIcon>
#include <KMessageBox>
#include <KLocalizedString>

#include <QUrl>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QDialogButtonBox>

SearchDialog::SearchDialog(QWidget *parent)
    : QDialog(parent)
    , mModel(new QStandardItemModel(this))
{
    setWindowTitle(i18n("Search"));
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    mUi.setupUi(mainWidget);
    mUi.credentialsGroup->setVisible(false);
    mUi.searchResults->setModel(mModel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SearchDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SearchDialog::reject);
    mainLayout->addWidget(buttonBox);
    buttonBox->button(QDialogButtonBox::Ok)->setText(i18n("Add Selected Items"));

    connect(mUi.searchUrl, &KLineEdit::textChanged, this, &SearchDialog::checkUserInput);
    connect(mUi.searchParam, &KLineEdit::textChanged, this, &SearchDialog::checkUserInput);
    connect(mUi.searchButton, &QPushButton::clicked, this, &SearchDialog::search);

    checkUserInput();
}

SearchDialog::~SearchDialog()
{
}

bool SearchDialog::useDefaultCredentials() const
{
    return mUi.useDefaultCreds->isChecked();
}

void SearchDialog::setUsername(const QString &user)
{
    mUi.username->setText(user);
}

QString SearchDialog::username() const
{
    return mUi.username->text();
}

void SearchDialog::setPassword(const QString &password)
{
    mUi.password->setText(password);
}

QString SearchDialog::password() const
{
    return mUi.password->text();
}

QStringList SearchDialog::selection() const
{
    const QModelIndexList indexes = mUi.searchResults->selectionModel()->selectedIndexes();
    QStringList ret;
    ret.reserve(indexes.count());
    for (const QModelIndex &index : indexes) {
        //qCritical() << "SELECTED DATA: " << index.data(Qt::UserRole + 1).toString();
        ret << index.data(Qt::UserRole + 1).toString();
    }
    return ret;
}

void SearchDialog::checkUserInput()
{
    if (mUi.searchUrl->text().isEmpty() || mUi.searchParam->text().isEmpty()) {
        mUi.searchButton->setEnabled(false);
    } else {
        mUi.searchButton->setEnabled(true);
    }
}

void SearchDialog::search()
{
    mUi.searchResults->setEnabled(false);
    mModel->clear();
    KDAV::DavPrincipalSearchJob::FilterType filter;

    if (mUi.searchType->currentIndex() == 0) {
        filter = KDAV::DavPrincipalSearchJob::DisplayName;
    } else {
        filter = KDAV::DavPrincipalSearchJob::EmailAddress;
    }

    QUrl url(mUi.searchUrl->text());
    url.setUserInfo(QString());
    KDAV::DavUrl davUrl;
    davUrl.setUrl(url);

    KDAV::DavPrincipalSearchJob *job = new KDAV::DavPrincipalSearchJob(davUrl, filter, mUi.searchParam->text(), this);
    job->fetchProperty(KDAV::ProtocolInfo::principalHomeSet(KDAV::CalDav), KDAV::ProtocolInfo::principalHomeSetNS(KDAV::CalDav));
    job->fetchProperty(KDAV::ProtocolInfo::principalHomeSet(KDAV::CardDav), KDAV::ProtocolInfo::principalHomeSetNS(KDAV::CardDav));

    connect(job, &KDAV::DavPrincipalSearchJob::result, this, &SearchDialog::onSearchJobFinished);
    job->start();
}

void SearchDialog::onSearchJobFinished(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(this, i18n("An error occurred when executing search:\n%1", job->errorText()));
        return;
    }

    KDAV::DavPrincipalSearchJob *davJob = qobject_cast<KDAV::DavPrincipalSearchJob *>(job);

    KDAV::DavUrl davUrl = davJob->davUrl();
    QUrl url = davUrl.url();

    const QVector<KDAV::DavPrincipalSearchJob::Result> results = davJob->results();
    for (const KDAV::DavPrincipalSearchJob::Result &result : results) {
        if (result.value.startsWith(QLatin1Char('/'))) {
            url.setPath(result.value, QUrl::TolerantMode);
        } else {
            QUrl tmp(result.value);
            tmp.setUserInfo(QString());
            url = tmp;
        }
        davUrl.setUrl(url);

        if (result.property == KDAV::ProtocolInfo::principalHomeSet(KDAV::CalDav)) {
            davUrl.setProtocol(KDAV::CalDav);
        } else {
            davUrl.setProtocol(KDAV::CardDav);
        }

        KDAV::DavCollectionsFetchJob *fetchJob = new KDAV::DavCollectionsFetchJob(davUrl);
        connect(fetchJob, &KDAV::DavCollectionsFetchJob::result, this, &SearchDialog::onCollectionsFetchJobFinished);
        fetchJob->start();
        ++mSubJobCount;
    }
}

void SearchDialog::onCollectionsFetchJobFinished(KJob *job)
{
    --mSubJobCount;

    if (mSubJobCount == 0) {
        mUi.searchResults->setEnabled(true);
    }

    if (job->error()) {
        if (mSubJobCount == 0) {
            KMessageBox::error(this, i18n("An error occurred when executing search:\n%1", job->errorText()));
        }
        return;
    }

    KDAV::DavCollectionsFetchJob *davJob = qobject_cast<KDAV::DavCollectionsFetchJob *>(job);
    const KDAV::DavCollection::List collections = davJob->collections();

    for (const KDAV::DavCollection &collection : collections) {
        QStandardItem *item = new QStandardItem(collection.displayName());
        QString data(KDAV::Utils::protocolName(collection.url().protocol()) + QLatin1Char('|') + collection.url().toDisplayString());
        item->setData(data, Qt::UserRole + 1);
        item->setToolTip(collection.url().toDisplayString());
        if (collection.url().protocol() == KDAV::CalDav) {
            item->setIcon(QIcon::fromTheme(QStringLiteral("view-calendar")));
        } else {
            item->setIcon(QIcon::fromTheme(QStringLiteral("view-pim-contacts")));
        }
        mModel->appendRow(item);
    }
}
