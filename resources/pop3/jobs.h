/*
   SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <KIO/MetaData>
#include <KJob>
#include <QUrl>

#include <KMime/Message>

#include <QObject>
#include <QPointer>

namespace KIO
{
class Slave;
class Job;
class TransferJob;
}

class SlaveBaseJob;
class Settings;

class POPSession : public QObject
{
    Q_OBJECT
public:
    explicit POPSession(Settings &settings, const QString &password);
    ~POPSession() override;
    bool connectSlave();

    void abortCurrentJob();
    void closeSession();

    KIO::Slave *getSlave() const;
    QUrl getUrl() const;

    // Sets the current SlaveBaseJob that is using the POPSession.
    // If there is a job, all slave errors will be forwared to that job
    void setCurrentJob(SlaveBaseJob *job);

private Q_SLOTS:
    void slotSlaveError(KIO::Slave *slave, int, const QString &);

Q_SIGNALS:

    // An error occurred within the slave. If there is a current job, this
    // signal is not emitted, as the job deals with it.
    void slaveError(int errorCode, const QString &errorMessage);

private:
    KIO::MetaData slaveConfig() const;
    QString authenticationToString(int type) const;

    QPointer<KIO::Slave> mSlave;
    SlaveBaseJob *mCurrentJob = nullptr;
    const QString mPassword;
    Settings &mSettings;
};

class SlaveBaseJob : public KJob
{
    Q_OBJECT

public:
    explicit SlaveBaseJob(POPSession *POPSession);
    ~SlaveBaseJob() override;

    virtual void slaveError(int errorCode, const QString &errorMessage);

protected Q_SLOTS:
    virtual void slotSlaveData(KIO::Job *job, const QByteArray &data);
    virtual void slotSlaveResult(KJob *job);

protected:
    QString errorString() const override;
    bool doKill() override;
    void startJob(const QString &path);
    virtual void connectJob();

    KIO::TransferJob *mJob = nullptr;
    POPSession *mPOPSession = nullptr;
};

class LoginJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    LoginJob(POPSession *popSession);
    void start() override;

protected:
    QString errorString() const override;

private Q_SLOTS:
    void slaveConnected(KIO::Slave *slave);

private:
    void slaveError(int errorCode, const QString &errorMessage) override;

    QString mErrorString;
};

class ListJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    ListJob(POPSession *popSession);
    QMap<int, int> idList() const;
    void start() override;

private:
    void slotSlaveData(KIO::Job *job, const QByteArray &data) override;

private:
    QMap<int, int> mIdList;
};

class UIDListJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    UIDListJob(POPSession *popSession);
    QMap<int, QString> uidList() const;
    QMap<QString, int> idList() const;
    void start() override;

private:
    void slotSlaveData(KIO::Job *job, const QByteArray &data) override;

    QMap<int, QString> mUidList;
    QMap<QString, int> mIdList;
};

class DeleteJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    DeleteJob(POPSession *popSession);
    void setDeleteIds(const QList<int> &ids);
    void start() override;
    QList<int> deletedIDs() const;

private:
    QList<int> mIdsToDelete;
};

class QuitJob : public SlaveBaseJob
{
    Q_OBJECT

public:
    QuitJob(POPSession *popSession);
    void start() override;
};

class FetchJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    FetchJob(POPSession *session);
    void setFetchIds(const QList<int> &ids, const QList<int> &sizes);
    void start() override;

private Q_SLOTS:
    void slotInfoMessage(KJob *job, const QString &infoMessage, const QString &);

Q_SIGNALS:
    void messageFinished(int id, KMime::Message::Ptr message);

private:
    void connectJob() override;
    void slotSlaveData(KIO::Job *job, const QByteArray &data) override;

    QList<int> mIdsPendingDownload;
    QByteArray mCurrentMessage;
    int mBytesDownloaded;
    int mTotalBytesToDownload;
    uint mDataCounter;
};

