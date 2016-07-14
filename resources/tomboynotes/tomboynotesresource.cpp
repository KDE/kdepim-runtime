/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "tomboynotesresource.h"
#include "debug.h"
#include "configdialog.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "tomboyserverauthenticatejob.h"
#include "tomboycollectionsdownloadjob.h"
#include "tomboyitemdownloadjob.h"
#include "tomboyitemsdownloadjob.h"
#include "tomboyitemuploadjob.h"
#include <changerecorder.h>
#include <ItemFetchScope>
#include <klocalizedstring.h>
#include <ksslinfodialog.h>
#include <kwindowsystem.h>
#include <QSslCipher>
#include <QtDBus/QDBusConnection>

using namespace Akonadi;

TomboyNotesResource::TomboyNotesResource(const QString &id)
    : ResourceBase(id)
{
    new SettingsAdaptor(Settings::self());
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 Settings::self(),
                                                 QDBusConnection::ExportAdaptors);

    // Akonadi:Item should always provide the payload
    changeRecorder()->itemFetchScope().fetchFullPayload(true);

    // Status message stuff
    mStatusMessageTimer = new QTimer(this);
    mStatusMessageTimer->setSingleShot(true);
    connect(mStatusMessageTimer, &QTimer::timeout, [ = ]() {
        Q_EMIT status(Akonadi::AgentBase::Idle, QString());
    });
    connect(this, &AgentBase::error, this, &TomboyNotesResource::showError);

    mUploadJobProcessRunning = false;

    mManager = new KIO::AccessManager(this);
    connect(mManager, &KIO::AccessManager::sslErrors, this, &TomboyNotesResource::onSslError);

    qCDebug(log_tomboynotesresource) << "Resource started";
}

TomboyNotesResource::~TomboyNotesResource()
{
}

void TomboyNotesResource::retrieveCollections()
{
    qCDebug(log_tomboynotesresource) << "Retrieving collections started";

    if (configurationNotValid()) {
        cancelTask(i18n("Resource configuration is not valid"));
        return;
    }

    auto job = new TomboyCollectionsDownloadJob(Settings::collectionName(), mManager, this);
    job->setAuthentication(Settings::requestToken(), Settings::requestTokenSecret());
    job->setServerURL(Settings::serverURL(), Settings::userURL());
    // connect to its result() signal
    connect(job, &KJob::result, this, &TomboyNotesResource::onCollectionsRetrieved);
    job->start();
    qCDebug(log_tomboynotesresource) << "Retriving collections job started";
}

void TomboyNotesResource::retrieveItems(const Akonadi::Collection &collection)
{
    if (configurationNotValid()) {
        cancelTask(i18n("Resource configuration is not valid"));
        return;
    }

    // create the job
    auto job = new TomboyItemsDownloadJob(collection.id(), mManager, this);
    job->setAuthentication(Settings::requestToken(), Settings::requestTokenSecret());
    job->setServerURL(Settings::serverURL(), Settings::contentURL());
    // connect to its result() signal
    connect(job, &KJob::result, this, &TomboyNotesResource::onItemsRetrieved);
    job->start();
    qCDebug(log_tomboynotesresource) << "Retriving items job started";
}

bool TomboyNotesResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts);

    if (configurationNotValid()) {
        cancelTask(i18n("Resource configuration is not valid"));
        return false;
    }

    // this method is called when Akonadi wants more data for a given item.
    auto job = new TomboyItemDownloadJob(item, mManager, this);
    job->setAuthentication(Settings::requestToken(), Settings::requestTokenSecret());
    job->setServerURL(Settings::serverURL(), Settings::contentURL());
    // connect to its result() signal
    connect(job, &KJob::result, this, &TomboyNotesResource::onItemRetrieved);
    job->start();
    qCDebug(log_tomboynotesresource) << "Retriving item data job started";

    return true;
}

void TomboyNotesResource::onAuthorizationFinished(KJob *kjob)
{
    // Saves the received client authentication data in the settings
    qCDebug(log_tomboynotesresource) << "Authorization job finished";
    auto job = qobject_cast<TomboyServerAuthenticateJob*>(kjob);
    if (job->error() == TomboyJobError::NoError) {
        Settings::setRequestToken(job->getRequestToken());
        Settings::setRequestTokenSecret(job->getRequestTokenSecret());
        Settings::setContentURL(job->getContentUrl());
        Settings::setUserURL(job->getUserURL());
        Settings::self()->save();
        synchronizeCollectionTree();
        synchronize();
    } else {
        showError(job->errorText());
    }
}

void TomboyNotesResource::onCollectionsRetrieved(KJob *kjob)
{
    auto job = qobject_cast<TomboyCollectionsDownloadJob*>(kjob);
    if (job->error() != TomboyJobError::NoError) {
        cancelTask();
        showError(job->errorText());
        return;
    }

    collectionsRetrieved(job->collections());
}

void TomboyNotesResource::onItemChangeCommitted(KJob *kjob)
{
    auto job = qobject_cast<TomboyItemUploadJob*>(kjob);
    mUploadJobProcessRunning = false;
    switch (job->error()) {
    case TomboyJobError::PermanentError:
        cancelTask();
        showError(job->errorText());
        return;
    case TomboyJobError::TemporaryError:
        retryAfterFailure(job->errorString());
        return;
    case TomboyJobError::NoError:
        changeCommitted(job->item());
        // The data should be actualized for the next UploadJob
        synchronize();
        return;
    }
}

void TomboyNotesResource::onItemRetrieved(KJob *kjob)
{
    auto job = qobject_cast<TomboyItemDownloadJob*>(kjob);

    if (job->error() != TomboyJobError::NoError) {
        cancelTask();
        showError(job->errorText());
        return;
    }

    itemRetrieved(job->item());
    qCDebug(log_tomboynotesresource) << "Retriving item data job with remoteId " << job->item().remoteId() << " finished";
}

void TomboyNotesResource::onItemsRetrieved(KJob *kjob)
{
    auto job = qobject_cast<TomboyItemsDownloadJob*>(kjob);
    if (job->error() != TomboyJobError::NoError) {
        cancelTask();
        showError(job->errorText());
        return;
    }

    itemsRetrieved(job->items());
    qCDebug(log_tomboynotesresource) << "Retriving items job finished";
}

void TomboyNotesResource::onSslError(QNetworkReply *reply, const QList<QSslError> &errors)
{
    if (Settings::ignoreSslErrors()) {
        reply->ignoreSslErrors();
    }
}

void TomboyNotesResource::aboutToQuit()
{
    // TODO: any cleanup you need to do while there is still an active
    // event loop. The resource will terminate after this method returns
}

void TomboyNotesResource::configure(WId windowId)
{
    qCDebug(log_tomboynotesresource) << "Resource configuration started";

    ConfigDialog dialog(Settings::self());

    if (windowId) {
        KWindowSystem::setMainWindow(&dialog, windowId);
    }

    // Run the configuration dialog an save settings if accepted
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    dialog.saveSettings();
    setAgentName(Settings::collectionName());

    if (configurationNotValid()) {
        auto job = new TomboyServerAuthenticateJob(mManager, this);
        job->setServerURL(Settings::serverURL(), QString());
        connect(job, &KJob::result, this, &TomboyNotesResource::onAuthorizationFinished);
        job->start();
        qCDebug(log_tomboynotesresource) << "Authorization job started";
    } else {
        synchronize();
    }
}

void TomboyNotesResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
    if (Settings::readOnly() || configurationNotValid()) {
        cancelTask(i18n("Resource is read-only"));
        return;
    }

    if (mUploadJobProcessRunning) {
        retryAfterFailure(QString());
        return;
    }

    auto job = new TomboyItemUploadJob(item, JobType::AddItem, mManager, this);
    job->setAuthentication(Settings::requestToken(), Settings::requestTokenSecret());
    job->setServerURL(Settings::serverURL(), Settings::contentURL());
    connect(job, &KJob::result, this, &TomboyNotesResource::onItemChangeCommitted);
    mUploadJobProcessRunning = true;
    job->start();
}

void TomboyNotesResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts);
    if (Settings::readOnly() || configurationNotValid()) {
        cancelTask(i18n("Resource is read-only"));
        return;
    }

    if (mUploadJobProcessRunning) {
        retryAfterFailure(QString());
        return;
    }

    auto job = new TomboyItemUploadJob(item, JobType::ModifyItem, mManager, this);
    job->setAuthentication(Settings::requestToken(), Settings::requestTokenSecret());
    job->setServerURL(Settings::serverURL(), Settings::contentURL());
    connect(job, &KJob::result, this, &TomboyNotesResource::onItemChangeCommitted);
    mUploadJobProcessRunning = true;
    job->start();
}

void TomboyNotesResource::itemRemoved(const Akonadi::Item &item)
{
    if (Settings::readOnly() || configurationNotValid()) {
        cancelTask(i18n("Resource is read-only"));
        return;
    }

    if (mUploadJobProcessRunning) {
        retryAfterFailure(QString());
        return;
    }

    auto job = new TomboyItemUploadJob(item, JobType::DeleteItem, mManager, this);
    job->setAuthentication(Settings::requestToken(), Settings::requestTokenSecret());
    job->setServerURL(Settings::serverURL(), Settings::contentURL());
    connect(job, &KJob::result, this, &TomboyNotesResource::onItemChangeCommitted);
    mUploadJobProcessRunning = true;
    job->start();
}

bool TomboyNotesResource::configurationNotValid() const
{
    return Settings::requestToken().isEmpty() || Settings::requestToken().isEmpty() || Settings::contentURL().isEmpty();
}

void TomboyNotesResource::retryAfterFailure(const QString &errorMessage)
{
    Q_EMIT status(Broken, errorMessage);
    deferTask();
    setTemporaryOffline(Settings::self()->refreshInterval() <= 0 ? 300 : Settings::self()->refreshInterval() * 60);
}

void TomboyNotesResource::showError(const QString &errorText)
{
    Q_EMIT status(Akonadi::AgentBase::Idle, errorText);
    mStatusMessageTimer->start(1000 * 10);
}

AKONADI_RESOURCE_MAIN(TomboyNotesResource)

#include "moc_tomboynotesresource.cpp"
