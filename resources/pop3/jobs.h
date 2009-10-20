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
#include <KUrl>
#include <KIO/MetaData>

#include <KMime/Message>

#include <QObject>
#include <QPointer>

#include <boost/shared_ptr.hpp>

namespace KIO
{
  class Slave;
  class Job;
  class SimpleJob;
}

class SlaveBaseJob;

class POPSession : public QObject
{
  Q_OBJECT
public:
  POPSession();
  ~POPSession();
  bool connectSlave();

  void abortCurrentJob();
  void closeSession();

  KIO::Slave* getSlave() const;
  KUrl getUrl() const;

  // Sets the current SlaveBaseJob that is using the POPSession.
  // If there is a job, all slave errors will be forwared to that job
  void setCurrentJob( SlaveBaseJob *job );

private slots:
  void slotSlaveError( KIO::Slave *slave , int, const QString & );

signals:

  // An error occurred within the slave. If there is a current job, this
  // signal is not emitted, as the job deals with it.
  void slaveError( int errorCode, const QString &errorMessage );

private:
  KIO::MetaData slaveConfig() const;

  QPointer<KIO::Slave> mSlave;
  SlaveBaseJob *mCurrentJob;
};


class SlaveBaseJob : public KJob
{
  Q_OBJECT

public:
  explicit SlaveBaseJob( POPSession *POPSession );
  ~SlaveBaseJob();

  virtual void slaveError( int errorCode, const QString &errorMessage );

protected slots:
  virtual void slotSlaveData( KIO::Job *job, const QByteArray &data );
  virtual void slotSlaveResult( KJob *job );

protected:
  virtual bool doKill();
  void startJob( const QString &path );
  virtual void connectJob();

  KIO::SimpleJob *mJob;
  POPSession *mPOPSession;
};

class LoginJob : public SlaveBaseJob
{
  Q_OBJECT
public:
  LoginJob( POPSession *popSession );
  virtual void start();

private slots:
  void slaveConnected( KIO::Slave *slave );

private:
  virtual void slaveError( int errorCode, const QString &errorMessage );
};


class ListJob : public SlaveBaseJob
{
  Q_OBJECT
public:
  ListJob( POPSession *popSession );
  QMap<int,int> idList() const;
  virtual void start();

private:
  virtual void slotSlaveData( KIO::Job *job, const QByteArray &data );

private:
  QMap<int,int> mIdList;
};


class UIDListJob : public SlaveBaseJob
{
  Q_OBJECT
public:
  UIDListJob( POPSession *popSession );
  QMap<int,QString> uidList() const;
  virtual void start();

private:
  virtual void slotSlaveData( KIO::Job *job, const QByteArray &data );

  QMap<int,QString> mUidList;
};

class DeleteJob : public SlaveBaseJob
{
  Q_OBJECT
public:
  DeleteJob( POPSession *popSession );
  void setDeleteIds( const QList<int> ids );
  virtual void start();
  QList<int> deletedIDs() const;

private:

  QList<int> mIdsToDelete;
};

class QuitJob : public SlaveBaseJob
{
  Q_OBJECT

public:
  QuitJob( POPSession *popSession );
  virtual void start();
};


class FetchJob : public SlaveBaseJob
{
  Q_OBJECT
public:

  FetchJob( POPSession *session );
  void setFetchIds( const QList<int> ids, QList<int> sizes );
  virtual void start();

private slots:
  void slotInfoMessage( KJob *job, const QString &infoMessage, const QString & );

signals:
  void messageFinished( int id, KMime::Message::Ptr message );

private:

  virtual void connectJob();
  virtual void slotSlaveData( KIO::Job *job, const QByteArray &data );

  QList<int> mIdsPendingDownload;
  QByteArray mCurrentMessage;
  int mBytesDownloaded;
  int mTotalBytesToDownload;
  uint mDataCounter;
};


#endif // JOBS_H
