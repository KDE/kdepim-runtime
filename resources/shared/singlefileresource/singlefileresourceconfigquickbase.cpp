// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "singlefileresourceconfigquickbase.h"

#include <KIO/Job>
#include <KIO/StatJob>
#include <KLocalizedString>
#include <KShell>

#include <QDir>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

SingleFileResourceConfigQuickBase::SingleFileResourceConfigQuickBase(const KSharedConfigPtr &config,
                                                                     const QString &identifier,
                                                                     KCoreConfigSkeleton *settings,
                                                                     const QStringList &nameFilters,
                                                                     QObject *parent)
    : AgentConfigurationQuickBase(config, identifier, parent)
    , mSettings(settings)
    , mNameFilters(nameFilters)
{
    mSettings->setParent(this);

    mStatTimer.setSingleShot(true);
    mStatTimer.setInterval(500ms);
    connect(&mStatTimer, &QTimer::timeout, this, &SingleFileResourceConfigQuickBase::startStat);
}

SingleFileResourceConfigQuickBase::~SingleFileResourceConfigQuickBase()
{
    if (mStatJob) {
        mStatJob->kill();
    }
}

QString SingleFileResourceConfigQuickBase::path() const
{
    return mSettings->findItem(u"Path"_s)->property().toString();
}

void SingleFileResourceConfigQuickBase::setPath(const QString &value)
{
    if (!mPathEditable) {
        return;
    }
    auto item = mSettings->findItem(u"Path"_s);
    if (item->isImmutable() || path() == value) {
        return;
    }
    item->setProperty(value);
    Q_EMIT settingsChanged();
    validate();
}

QString SingleFileResourceConfigQuickBase::displayName() const
{
    return mSettings->findItem(u"DisplayName"_s)->property().toString();
}

void SingleFileResourceConfigQuickBase::setDisplayName(const QString &value)
{
    auto item = mSettings->findItem(u"DisplayName"_s);
    if (item->isImmutable() || displayName() == value) {
        return;
    }
    item->setProperty(value);
    Q_EMIT settingsChanged();
}

bool SingleFileResourceConfigQuickBase::readOnly() const
{
    return mSettings->findItem(u"ReadOnly"_s)->property().toBool();
}

void SingleFileResourceConfigQuickBase::setReadOnly(bool value)
{
    if (!localFile()) {
        value = true;
    }
    auto item = mSettings->findItem(u"ReadOnly"_s);
    if (item->isImmutable() || readOnly() == value) {
        return;
    }
    item->setProperty(value);
    Q_EMIT settingsChanged();
}

bool SingleFileResourceConfigQuickBase::monitorFile() const
{
    return mSettings->findItem(u"MonitorFile"_s)->property().toBool();
}

void SingleFileResourceConfigQuickBase::setMonitorFile(bool value)
{
    if (!localFile()) {
        value = false;
    }
    auto item = mSettings->findItem(u"MonitorFile"_s);
    if (item->isImmutable() || monitorFile() == value) {
        return;
    }
    item->setProperty(value);
    Q_EMIT settingsChanged();
}

bool SingleFileResourceConfigQuickBase::periodicUpdate() const
{
    return mSettings->findItem(u"PeriodicUpdate"_s)->property().toBool();
}

void SingleFileResourceConfigQuickBase::setPeriodicUpdate(bool value)
{
    auto item = mSettings->findItem(u"PeriodicUpdate"_s);
    if (item->isImmutable() || periodicUpdate() == value) {
        return;
    }
    item->setProperty(value);
    Q_EMIT settingsChanged();
}

int SingleFileResourceConfigQuickBase::updatePeriod() const
{
    return mSettings->findItem(u"UpdatePeriod"_s)->property().toInt();
}

void SingleFileResourceConfigQuickBase::setUpdatePeriod(int value)
{
    value = qBound(1, value, 1440);
    auto item = mSettings->findItem(u"UpdatePeriod"_s);
    if (item->isImmutable() || updatePeriod() == value) {
        return;
    }
    item->setProperty(value);
    Q_EMIT settingsChanged();
}

bool SingleFileResourceConfigQuickBase::pathEditable() const
{
    return mPathEditable && !mSettings->findItem(u"Path"_s)->isImmutable();
}

bool SingleFileResourceConfigQuickBase::localFile() const
{
    return path().trimmed().isEmpty() || url().isLocalFile();
}

QUrl SingleFileResourceConfigQuickBase::url() const
{
    const QString input = KShell::tildeExpand(path().trimmed());
    return QUrl::fromUserInput(input, QDir::homePath(), QUrl::AssumeLocalFile);
}

QString SingleFileResourceConfigQuickBase::status() const
{
    return mStatus;
}

bool SingleFileResourceConfigQuickBase::statusIsError() const
{
    return mStatusIsError;
}

QStringList SingleFileResourceConfigQuickBase::nameFilters() const
{
    return mNameFilters;
}

void SingleFileResourceConfigQuickBase::setFileUrl(const QUrl &url)
{
    setPath(url.toString(QUrl::PreferLocalFile));
}

void SingleFileResourceConfigQuickBase::load()
{
    AgentConfigurationQuickBase::load();
    mSettings->load();
    mPathEditable = path().isEmpty();
    mRemoteMode = !localFile();
    mLocalReadOnly = mRemoteMode ? false : readOnly();
    mLocalMonitorFile = mRemoteMode ? true : monitorFile();
    Q_EMIT settingsChanged();
    validate();
}

bool SingleFileResourceConfigQuickBase::save()
{
    if (!mValid) {
        return false;
    }
    if (mPathEditable) {
        auto item = mSettings->findItem(u"Path"_s);
        if (!item->isImmutable()) {
            item->setProperty(url().toString(QUrl::PreferLocalFile));
        }
    }
    if (!mSettings->save()) {
        return false;
    }
    return AgentConfigurationQuickBase::save();
}

void SingleFileResourceConfigQuickBase::setValid(bool valid)
{
    mValid = valid;
    Q_EMIT enableOkButton(valid);
    Q_EMIT statusChanged();
}

void SingleFileResourceConfigQuickBase::setRemoteMode(bool remote)
{
    if (mRemoteMode == remote) {
        return;
    }
    if (remote) {
        mLocalReadOnly = readOnly();
        mLocalMonitorFile = monitorFile();
        mRemoteMode = true;
        setReadOnly(true);
        setMonitorFile(false);
    } else {
        mRemoteMode = false;
        setReadOnly(mLocalReadOnly);
        setMonitorFile(mLocalMonitorFile);
    }
}

void SingleFileResourceConfigQuickBase::validate()
{
    mStatTimer.stop();
    if (mStatJob) {
        mStatJob->kill();
        mStatJob = nullptr;
    }
    mCheckingParent = false;
    mStatus.clear();
    mStatusIsError = false;

    const QUrl currentUrl = url();
    if (path().trimmed().isEmpty() || !currentUrl.isValid()) {
        setRemoteMode(false);
        setValid(false);
        return;
    }
    if (currentUrl.isLocalFile()) {
        setRemoteMode(false);
        setValid(true);
        return;
    }
    setRemoteMode(true);
    mStatus = i18ndc("akonadi_singlefile_resource", "@info:status", "Checking file information…");
    setValid(false);
    mStatTimer.start();
}

void SingleFileResourceConfigQuickBase::startStat()
{
    mStatJob = KIO::stat(url(), KIO::StatJob::SourceSide, KIO::StatDetail::StatDefaultDetails, KIO::DefaultFlags | KIO::HideProgressInfo);
    connect(mStatJob, &KIO::StatJob::result, this, &SingleFileResourceConfigQuickBase::statFinished);
}

void SingleFileResourceConfigQuickBase::statFinished(KJob *job)
{
    if (job != mStatJob) {
        return;
    }
    if (job->error() == KIO::ERR_DOES_NOT_EXIST && !mCheckingParent) {
        mCheckingParent = true;
        mStatJob = KIO::stat(KIO::upUrl(url()), KIO::StatJob::SourceSide, KIO::StatDetail::StatDefaultDetails, KIO::DefaultFlags | KIO::HideProgressInfo);
        connect(mStatJob, &KIO::StatJob::result, this, &SingleFileResourceConfigQuickBase::statFinished);
        return;
    }
    mStatus = job->error() ? job->errorString() : QString();
    mStatusIsError = job->error() != 0;
    mStatJob = nullptr;
    setValid(!job->error());
}

#include "moc_singlefileresourceconfigquickbase.cpp"
