#ifndef MBOX_P_H
#define MBOX_P_H

#include "mbox.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QTimer>

class MBoxPrivate : public QObject
{
  Q_OBJECT;

  public:
    MBoxPrivate(MBox *mbox) : mInitialMboxFileSize( 0 ), mMBox(mbox)
    {
      qDebug() << "MBOX PRIVATE:";
      connect(&mUnlockTimer, SIGNAL(timeout()), SLOT(unlockMBox()));
    }

    virtual ~MBoxPrivate()
    {
      if ( mMboxFile.isOpen() )
        mMboxFile.close();
    }

    void close()
    {
      if ( mMboxFile.isOpen() )
        mMboxFile.close();

      mFileLocked = false;
    }

    bool startTimerIfNeeded()
    {
      if (mUnlockTimer.interval() > 0) {
        mUnlockTimer.start();
        return true;
      }

      return false;
    }

  public Q_SLOTS:
    void unlockMBox()
    {
      qDebug() <<  "TIMED OUT!";
      mMBox->unlock();
    }

  public:
    QByteArray     mAppendedEntries;
    QList<MsgInfo> mEntries;
    bool           mFileLocked;
    quint64        mInitialMboxFileSize;
    QString        mLockFileName;
    MBox::LockType mLockType;
    MBox          *mMBox;
    QFile          mMboxFile;
    bool           mReadOnly;
    QTimer         mUnlockTimer;
};

#endif // MBOX_P_H
