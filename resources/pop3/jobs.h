/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef JOBS_H
#define JOBS_H

#include <KJob>
#include <QUrl>
#include <KIO/MetaData>

#include <KMime/Message>

#include <QObject>
#include <QPointer>

#include <boost/shared_ptr.hpp>

namespace KIO
{
class Slave;
class Job;
class TransferJob;
}

class SlaveBaseJob;

class POPSession : public QObject
{
    Q_OBJECT
public:
    explicit POPSession(const QString &password);
    ~POPSession();
    bool connectSlave();

    void abortCurrentJob();
    void closeSession();

    KIO::Slave *getSlave() const;
    QUrl getUrl() const;

    // Sets the current SlaveBaseJob that is using the POPSession.
    // If there is a job, all slave errors will be forwared to that job
    void setCurrentJob(SlaveBaseJob *job);

private Q_SLOTS:
    void slotSlaveError(KIO::Slave *slave , int, const QString &);

Q_SIGNALS:

    // An error occurred within the slave. If there is a current job, this
    // signal is not emitted, as the job deals with it.
    void slaveError(int errorCode, const QString &errorMessage);

private:
    KIO::MetaData slaveConfig() const;
    QString authenticationToString(int type) const;

    QPointer<KIO::Slave> mSlave;
    SlaveBaseJob *mCurrentJob;
    QString mPassword;
};

class SlaveBaseJob : public KJob
{
    Q_OBJECT

public:
    explicit SlaveBaseJob(POPSession *POPSession);
    ~SlaveBaseJob();

    virtual void slaveError(int errorCode, const QString &errorMessage);

protected Q_SLOTS:
    virtual void slotSlaveData(KIO::Job *job, const QByteArray &data);
    virtual void slotSlaveResult(KJob *job);

protected:
    QString errorString() const Q_DECL_OVERRIDE;
    bool doKill() Q_DECL_OVERRIDE;
    void startJob(const QString &path);
    virtual void connectJob();

    KIO::TransferJob *mJob;
    POPSession *mPOPSession;
};

class LoginJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    LoginJob(POPSession *popSession);
    void start() Q_DECL_OVERRIDE;

protected:
    QString errorString() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slaveConnected(KIO::Slave *slave);

private:
    void slaveError(int errorCode, const QString &errorMessage) Q_DECL_OVERRIDE;

    QString mErrorString;
};

class ListJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    ListJob(POPSession *popSession);
    QMap<int, int> idList() const;
    void start() Q_DECL_OVERRIDE;

private:
    void slotSlaveData(KIO::Job *job, const QByteArray &data) Q_DECL_OVERRIDE;

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
    void start() Q_DECL_OVERRIDE;

private:
    void slotSlaveData(KIO::Job *job, const QByteArray &data) Q_DECL_OVERRIDE;

    QMap<int, QString> mUidList;
    QMap<QString, int> mIdList;
};

class DeleteJob : public SlaveBaseJob
{
    Q_OBJECT
public:
    DeleteJob(POPSession *popSession);
    void setDeleteIds(const QList<int> &ids);
    void start() Q_DECL_OVERRIDE;
    QList<int> deletedIDs() const;

private:

    QList<int> mIdsToDelete;
};

class QuitJob : public SlaveBaseJob
{
    Q_OBJECT

public:
    QuitJob(POPSession *popSession);
    void start() Q_DECL_OVERRIDE;
};

class FetchJob : public SlaveBaseJob
{
    Q_OBJECT
public:

    FetchJob(POPSession *session);
    void setFetchIds(const QList<int> &ids, const QList<int> &sizes);
    void start() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotInfoMessage(KJob *job, const QString &infoMessage, const QString &);

Q_SIGNALS:
    void messageFinished(int id, KMime::Message::Ptr message);

private:

    void connectJob() Q_DECL_OVERRIDE;
    void slotSlaveData(KIO::Job *job, const QByteArray &data) Q_DECL_OVERRIDE;

    QList<int> mIdsPendingDownload;
    QByteArray mCurrentMessage;
    int mBytesDownloaded;
    int mTotalBytesToDownload;
    uint mDataCounter;
};

#endif // JOBS_H
