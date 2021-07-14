/*
   SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "jobs.h"
#include "settings.h"

#include <MailTransport/Transport>

#include "pop3resource_debug.h"
#include <KIO/Job>
#include <KIO/Scheduler>
#include <KIO/Slave>
#include <KIO/TransferJob>
#include <KLocalizedString>

POPSession::POPSession(Settings &settings, const QString &password)
    : mPassword(password)
    , mSettings(settings)
{
    KIO::Scheduler::connect(SIGNAL(slaveError(KIO::Slave *, int, QString)), this, SLOT(slotSlaveError(KIO::Slave *, int, QString)));
}

POPSession::~POPSession()
{
    closeSession();
}

void POPSession::slotSlaveError(KIO::Slave *slave, int errorCode, const QString &errorMessage)
{
    Q_UNUSED(slave)
    qCWarning(POP3RESOURCE_LOG) << "Got a slave error:" << errorMessage;

    if (slave != mSlave) {
        return;
    }

    if (errorCode == KIO::ERR_SLAVE_DIED) {
        mSlave = nullptr;
    }

    // Explicitly disconnect the slave if the connection went down
    if (errorCode == KIO::ERR_CONNECTION_BROKEN && mSlave) {
        KIO::Scheduler::disconnectSlave(mSlave);
        mSlave = nullptr;
    }

    if (!mCurrentJob) {
        Q_EMIT slaveError(errorCode, errorMessage);
    } else {
        // Let the job deal with the problem
        mCurrentJob->slaveError(errorCode, errorMessage);
    }
}

void POPSession::setCurrentJob(SlaveBaseJob *job)
{
    mCurrentJob = job;
}

KIO::MetaData POPSession::slaveConfig() const
{
    KIO::MetaData m;

    m.insert(QStringLiteral("progress"), QStringLiteral("off"));
    m.insert(QStringLiteral("tls"), mSettings.useTLS() ? QStringLiteral("on") : QStringLiteral("off"));
    m.insert(QStringLiteral("pipelining"), (mSettings.pipelining()) ? QStringLiteral("on") : QStringLiteral("off"));
    m.insert(QStringLiteral("useProxy"), mSettings.useProxy() ? QStringLiteral("on") : QStringLiteral("off"));
    int type = mSettings.authenticationMethod();
    switch (type) {
    case MailTransport::Transport::EnumAuthenticationType::PLAIN:
    case MailTransport::Transport::EnumAuthenticationType::LOGIN:
    case MailTransport::Transport::EnumAuthenticationType::CRAM_MD5:
    case MailTransport::Transport::EnumAuthenticationType::DIGEST_MD5:
    case MailTransport::Transport::EnumAuthenticationType::NTLM:
    case MailTransport::Transport::EnumAuthenticationType::GSSAPI:
        m.insert(QStringLiteral("auth"), QStringLiteral("SASL"));
        m.insert(QStringLiteral("sasl"), authenticationToString(type));
        break;
    case MailTransport::Transport::EnumAuthenticationType::CLEAR:
        m.insert(QStringLiteral("auth"), QStringLiteral("USER"));
        break;
    default:
        m.insert(QStringLiteral("auth"), authenticationToString(type));
        break;
    }

    return m;
}

QString POPSession::authenticationToString(int type) const
{
    switch (type) {
    case MailTransport::Transport::EnumAuthenticationType::LOGIN:
        return QStringLiteral("LOGIN");
    case MailTransport::Transport::EnumAuthenticationType::PLAIN:
        return QStringLiteral("PLAIN");
    case MailTransport::Transport::EnumAuthenticationType::CRAM_MD5:
        return QStringLiteral("CRAM-MD5");
    case MailTransport::Transport::EnumAuthenticationType::DIGEST_MD5:
        return QStringLiteral("DIGEST-MD5");
    case MailTransport::Transport::EnumAuthenticationType::GSSAPI:
        return QStringLiteral("GSSAPI");
    case MailTransport::Transport::EnumAuthenticationType::NTLM:
        return QStringLiteral("NTLM");
    case MailTransport::Transport::EnumAuthenticationType::CLEAR:
        return QStringLiteral("USER");
    case MailTransport::Transport::EnumAuthenticationType::APOP:
        return QStringLiteral("APOP");
    default:
        break;
    }
    return QString();
}

QUrl POPSession::getUrl() const
{
    QUrl url;

    if (mSettings.useSSL()) {
        url.setScheme(QStringLiteral("pop3s"));
    } else {
        url.setScheme(QStringLiteral("pop3"));
    }

    url.setUserName(mSettings.login());
    url.setPassword(mPassword);
    url.setHost(mSettings.host());
    url.setPort(mSettings.port());
    return url;
}

bool POPSession::connectSlave()
{
    mSlave = KIO::Scheduler::getConnectedSlave(getUrl(), slaveConfig());
    return mSlave != nullptr;
}

void POPSession::abortCurrentJob()
{
    if (mCurrentJob) {
        mCurrentJob->kill(KJob::Quietly);
        mCurrentJob = nullptr;
    }
}

void POPSession::closeSession()
{
    if (mSlave) {
        KIO::Scheduler::disconnectSlave(mSlave);
    }
}

KIO::Slave *POPSession::getSlave() const
{
    return mSlave;
}

static QByteArray cleanupListRespone(const QByteArray &response)
{
    QByteArray ret = response.simplified(); // Workaround for Maillennium POP3/UNIBOX

    // Get rid of the null terminating character, if it exists
    int retSize = ret.size();
    if (retSize > 0 && ret.at(retSize - 1) == 0) {
        ret.chop(1);
    }
    return ret;
}

static QString intListToString(const QList<int> &intList)
{
    QString idList;
    for (int id : intList) {
        idList += QString::number(id) + QLatin1Char(',');
    }
    idList.chop(1);
    return idList;
}

SlaveBaseJob::SlaveBaseJob(POPSession *POPSession)
    : mJob(nullptr)
    , mPOPSession(POPSession)
{
    mPOPSession->setCurrentJob(this);
}

SlaveBaseJob::~SlaveBaseJob()
{
    // Don't do that here, the job might be destroyed after another one was started
    // and therefore overwrite the current job
    // mPOPSession->setCurrentJob( 0 );
}

bool SlaveBaseJob::doKill()
{
    if (mJob) {
        return mJob->kill();
    } else {
        return KJob::doKill();
    }
}

void SlaveBaseJob::slotSlaveResult(KJob *job)
{
    mPOPSession->setCurrentJob(nullptr);
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    }
    emitResult();
    mJob = nullptr;
}

void SlaveBaseJob::slotSlaveData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)
    qCWarning(POP3RESOURCE_LOG) << "Got unexpected slave data:" << data.data();
}

void SlaveBaseJob::slaveError(int errorCode, const QString &errorMessage)
{
    // The slave experienced some problem while running our job.
    // Just treat this as an error.
    // Derived jobs can do something more sophisticated here
    setError(errorCode);
    setErrorText(errorMessage);
    emitResult();
    mJob = nullptr;
}

void SlaveBaseJob::connectJob()
{
    connect(mJob, &KIO::TransferJob::data, this, &SlaveBaseJob::slotSlaveData);
    connect(mJob, &KIO::TransferJob::result, this, &SlaveBaseJob::slotSlaveResult);
}

void SlaveBaseJob::startJob(const QString &path)
{
    QUrl url = mPOPSession->getUrl();
    url.setPath(path);
    mJob = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
    KIO::Scheduler::assignJobToSlave(mPOPSession->getSlave(), mJob);
    connectJob();
}

QString SlaveBaseJob::errorString() const
{
    if (mJob) {
        return mJob->errorString();
    } else {
        return KJob::errorString();
    }
}

LoginJob::LoginJob(POPSession *popSession)
    : SlaveBaseJob(popSession)
{
}

void LoginJob::start()
{
    // This will create a connected slave, which means it will also try to login.
    KIO::Scheduler::connect(SIGNAL(slaveConnected(KIO::Slave *)), this, SLOT(slaveConnected(KIO::Slave *)));
    if (!mPOPSession->connectSlave()) {
        setError(KJob::UserDefinedError);
        setErrorText(i18n("Unable to create POP3 slave, aborting mail check."));
        emitResult();
    }
}

void LoginJob::slaveConnected(KIO::Slave *slave)
{
    if (slave != mPOPSession->getSlave()) {
        // Odd, not our slave...
        return;
    }

    // Yeah it connected, so login was successful!
    emitResult();
}

void LoginJob::slaveError(int errorCode, const QString &errorMessage)
{
    setError(errorCode);
    setErrorText(errorMessage);
    mErrorString = KIO::buildErrorString(errorCode, errorMessage);
    emitResult();
}

QString LoginJob::errorString() const
{
    return mErrorString;
}

ListJob::ListJob(POPSession *popSession)
    : SlaveBaseJob(popSession)
{
}

void ListJob::start()
{
    startJob(QStringLiteral("/index"));
}

void ListJob::slotSlaveData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)

    // Silly slave, why are you sending us empty data?
    if (data.isEmpty()) {
        return;
    }

    QByteArray cleanData = cleanupListRespone(data);
    const int space = cleanData.indexOf(' ');

    if (space > 0) {
        QByteArray lengthString = cleanData.mid(space + 1);
        const int spaceInLengthPos = lengthString.indexOf(' ');
        if (spaceInLengthPos != -1) {
            lengthString.truncate(spaceInLengthPos);
        }
        const int length = lengthString.toInt();

        QByteArray idString = cleanData.left(space);

        bool idIsNumber;
        int id = QString::fromLatin1(idString).toInt(&idIsNumber);
        if (idIsNumber) {
            mIdList.insert(id, length);
        } else {
            qCWarning(POP3RESOURCE_LOG) << "Got non-integer ID as part of the LIST response, ignoring" << idString.data();
        }
    } else {
        qCWarning(POP3RESOURCE_LOG) << "Got invalid LIST response:" << data.data();
    }
}

QMap<int, int> ListJob::idList() const
{
    return mIdList;
}

UIDListJob::UIDListJob(POPSession *popSession)
    : SlaveBaseJob(popSession)
{
}

void UIDListJob::start()
{
    startJob(QStringLiteral("/uidl"));
}

void UIDListJob::slotSlaveData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)

    // Silly slave, why are you sending us empty data?
    if (data.isEmpty()) {
        return;
    }

    QByteArray cleanData = cleanupListRespone(data);
    const int space = cleanData.indexOf(' ');

    if (space <= 0) {
        qCWarning(POP3RESOURCE_LOG) << "Invalid response to the UIDL command:" << data.data();
        qCWarning(POP3RESOURCE_LOG) << "Ignoring this entry.";
    } else {
        QByteArray idString = cleanData.left(space);
        QByteArray uidString = cleanData.mid(space + 1);
        bool idIsNumber;
        int id = QString::fromLatin1(idString).toInt(&idIsNumber);
        if (idIsNumber) {
            const QString uidQString = QString::fromLatin1(uidString);
            if (!uidQString.isEmpty()) {
                mUidList.insert(id, uidQString);
                mIdList.insert(uidQString, id);
            } else {
                qCWarning(POP3RESOURCE_LOG) << "Got invalid/empty UID from the UIDL command:" << uidString.data();
                qCWarning(POP3RESOURCE_LOG) << "The whole response was:" << data.data();
            }
        } else {
            qCWarning(POP3RESOURCE_LOG) << "Got invalid ID from the UIDL command:" << idString.data();
            qCWarning(POP3RESOURCE_LOG) << "The whole response was:" << data.data();
        }
    }
}

QMap<int, QString> UIDListJob::uidList() const
{
    return mUidList;
}

QMap<QString, int> UIDListJob::idList() const
{
    return mIdList;
}

DeleteJob::DeleteJob(POPSession *popSession)
    : SlaveBaseJob(popSession)
{
}

void DeleteJob::setDeleteIds(const QList<int> &ids)
{
    mIdsToDelete = ids;
}

void DeleteJob::start()
{
    qCDebug(POP3RESOURCE_LOG) << "================= DeleteJob::start. =============================";
    startJob(QLatin1String("/remove/") + intListToString(mIdsToDelete));
}

QList<int> DeleteJob::deletedIDs() const
{
    // FIXME : The slave doesn't tell us which of the IDs were actually deleted, we
    //         just assume all of them here
    return mIdsToDelete;
}

QuitJob::QuitJob(POPSession *popSession)
    : SlaveBaseJob(popSession)
{
}

void QuitJob::start()
{
    startJob(QStringLiteral("/commit"));
}

FetchJob::FetchJob(POPSession *session)
    : SlaveBaseJob(session)
    , mBytesDownloaded(0)
    , mTotalBytesToDownload(0)
    , mDataCounter(0)
{
}

void FetchJob::setFetchIds(const QList<int> &ids, const QList<int> &sizes)
{
    mIdsPendingDownload = ids;
    for (int size : std::as_const(sizes)) {
        mTotalBytesToDownload += size;
    }
}

void FetchJob::start()
{
    startJob(QLatin1String("/download/") + intListToString(mIdsPendingDownload));
    setTotalAmount(KJob::Bytes, mTotalBytesToDownload);
}

void FetchJob::connectJob()
{
    SlaveBaseJob::connectJob();
    connect(mJob, &KIO::TransferJob::infoMessage, this, &FetchJob::slotInfoMessage);
}

void FetchJob::slotSlaveData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)
    mCurrentMessage += data;
    mBytesDownloaded += data.size();
    mDataCounter++;
    if (mDataCounter % 5 == 0) {
        setProcessedAmount(KJob::Bytes, mBytesDownloaded);
    }
}

void FetchJob::slotInfoMessage(KJob *job, const QString &infoMessage, const QString &)
{
    Q_UNUSED(job)
    if (infoMessage != QLatin1String("message complete")) {
        return;
    }

    KMime::Message::Ptr msg(new KMime::Message);
    msg->setContent(KMime::CRLFtoLF(mCurrentMessage));
    msg->parse();

    mCurrentMessage.clear();
    const int idOfCurrentMessage = mIdsPendingDownload.takeFirst();
    Q_EMIT messageFinished(idOfCurrentMessage, msg);
}
