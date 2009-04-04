/*
    Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
    Copyright (c) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

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

    NOTE: Most of the code inside here is an slightly adjusted version of
          kdepim/kmail/kmfoldermbox.cpp. This is why I added a copyright line
          for Stefan Taferner.

          Bertjan Broeksema, april 2009
*/

#include "mbox.h"

#include <errno.h>
#include <kde_file.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <QtCore/QFileInfo>

class MBox::Private
{
  public:
    Private(const QString &mboxFile, bool readOnly)
      : mMboxFile(mboxFile), mReadOnly(readOnly)
    {}

    ~Private()
    {
      if (mStream) // shouldn't happen.
        fclose(mStream);
    }

    bool mFileLocked;
    bool mFileOpen;
    LockType mLockType;
    QString mMboxFile;
    QString mProcmailLockFileName;
    bool mReadOnly;
    FILE* mStream;
};

/// public methods.

MBox::MBox(const QString &mboxFile, bool readOnly)
  : d(new Private(mboxFile, readOnly))
{
  // Set some sane defaults
  d->mFileLocked = false;
  d->mFileOpen = false;
  d->mLockType = FCNTL;
  d->mStream = 0;
}

MBox::~MBox()
{
  if (d->mFileOpen)
    close();

  delete d;
}

void MBox::close()
{
  if (d->mFileLocked)
    unlock();

  if (d->mStream)
    fclose(d->mStream);

  d->mFileLocked = false;
  d->mFileOpen = false;
  d->mStream = 0;
}

QStringList MBox::entryList(const QSet<int> &deletedItems) const
{
  Q_UNUSED(deletedItems);
  // FIXME: Implement.
  return QStringList();
}

bool MBox::isValid() const
{
  QString msg;
  return isValid(msg);
}

bool MBox::isValid(QString &errorMsg) const
{
  QFileInfo info(d->mMboxFile);

  if (!info.isFile()) {
    errorMsg = i18n("%1 is not a file.").arg(d->mMboxFile);
    return false;
  }

  if (!info.exists()) {
    errorMsg = i18n("%1 does not exist").arg(d->mMboxFile);
    return false;
  }

  switch (d->mLockType) {
    case procmail_lockfile:
      if (KStandardDirs::findExe("lockfile").isEmpty()) {
        errorMsg = i18n("Could not find the lockfile executable");
        return false;
      }
      break;
    case mutt_dotlock: // fall through
    case mutt_dotlock_privileged:
      if (KStandardDirs::findExe("mutt_dotlock").isEmpty()) {
        errorMsg = i18n("Could not find the mutt_dotlock executable");
        return false;
      }
      break;
    default:
      break; // We assume fcntl available and lock_none doesn't need a check.
  }

  // TODO: Add some heuristics to see if the file actually is a mbox file.

  return true;
}

int MBox::open(OpenMode openMode)
{
  if (d->mFileOpen && openMode == Normal) {
    return 0;  // already open
  } else if (d->mFileOpen) {
    close(); // ReloadMode, so close the file first. 
  }

  d->mFileLocked = false;
  d->mStream = KDE_fopen( QFile::encodeName( d->mMboxFile ), "r+" ); // messages file
  if (!d->mStream) {
    kDebug() << "Cannot open mbox file `" << d->mMboxFile << "':" << strerror(errno);
    d->mFileOpen = false;
    return errno;
  }

  if (int rc = lock() != 0)
    return rc;

  fcntl(fileno(d->mStream), F_SETFD, FD_CLOEXEC);

  return errno;
}

void MBox::setLockType(LockType ltype)
{
  d->mLockType = ltype;
}

void MBox::setProcmailLockFile(const QString &lockFile)
{
  d->mProcmailLockFileName = lockFile;
}

/// private methods

int MBox::lock()
{
#ifdef Q_WS_WIN
# ifdef __GNUC__
#  warning TODO implement mbox locking on Windows
# else
#  pragma WARNING( TODO implement mbox locking on Windows )
# endif
  Q_ASSERT(d->mStream != 0);
  d->mFileLocked = true;
  d->mReadOnly = false;
#else
  Q_ASSERT(d->mStream != 0);

  d->mFileLocked = false;
  int rc = 0;
  QByteArray cmd_str;

  switch(d->mLockType)
  {
    case FCNTL:
      struct flock fl;
      fl.l_type   = F_WRLCK;
      fl.l_whence = 0;
      fl.l_start  = 0;
      fl.l_len    = 0;
      fl.l_pid    = -1;
      // Set the lock, optionally wait until a previous set lock is released.
      rc = fcntl(fileno(d->mStream), F_SETLKW, &fl);

      if (rc < 0) {
        kDebug() << "Cannot lock mbox file `" << d->mMboxFile << "':"
                 << strerror(errno) << "(" << errno << ")"
                 << " switching to read only mode";
        d->mReadOnly = true; // In case the MBox object was created read/write we
                             // set it to read only when locking failed.
        return errno;
      }
      break;

    case procmail_lockfile:
      cmd_str = "lockfile -l20 -r5 ";

      if (!d->mProcmailLockFileName.isEmpty())
        cmd_str += QFile::encodeName(KShell::quoteArg(d->mProcmailLockFileName));
      else
        cmd_str += QFile::encodeName(KShell::quoteArg(d->mMboxFile + ".lock"));

      rc = system(cmd_str.data());
      if(rc != 0) {
        kDebug() << "Cannot lock mbox file `" << d->mMboxFile << "':"
                 << strerror(rc) << "(" << rc << ")"
                 << " switching to read only mode";
        d->mReadOnly = true; // In case the MBox object was created read/write we
                             // set it to read only when locking failed.
        return rc;
      }
      break;

    case mutt_dotlock:
      cmd_str = "mutt_dotlock "
        + QFile::encodeName(KShell::quoteArg(d->mMboxFile));
      rc = system( cmd_str.data() );

      if(rc != 0) {
        kDebug() << "Cannot lock mbox file `" << d->mMboxFile << "':"
                 << strerror(rc) << "(" << rc << ")"
                 << " switching to read only mode";
        d->mReadOnly = true; // In case the MBox object was created read/write we
                          // set it to read only when locking failed.
        return rc;
      }
      break;

    case mutt_dotlock_privileged:
      cmd_str = "mutt_dotlock -p "
        + QFile::encodeName(KShell::quoteArg(d->mMboxFile));
      rc = system(cmd_str.data());

      if(rc != 0) {
        kDebug() << "Cannot lock mbox file `" << d->mMboxFile << "':"
                 << strerror(rc) << "(" << rc << ")"
                 << " switching to read only mode";
        d->mReadOnly = true;
        return rc;
      }
      break;

    case lock_none: // Fall through
    default:
      break;
  }

  d->mFileLocked = true;
#endif
  return 0;
}

int MBox::unlock()
{
  #ifdef Q_WS_WIN
# ifdef __GNUC__
#  warning TODO implement mbox unlocking on Windows
# else
#  pragma WARNING( TODO implement mbox unlocking on Windows )
# endif
  mFilesLocked = false;
  return 0;
#else
  int rc;
  struct flock fl;
  fl.l_type=F_UNLCK;
  fl.l_whence=0;
  fl.l_start=0;
  fl.l_len=0;
  QByteArray cmd_str;

  Q_ASSERT(d->mStream != 0);
  d->mFileLocked = false;

  switch(d->mLockType)
  {
    case FCNTL:
      fcntl(fileno(d->mStream), F_SETLK, &fl);
      rc = errno;
      break;

    case procmail_lockfile:
      cmd_str = "rm -f ";
      if (!d->mProcmailLockFileName.isEmpty())
        cmd_str += QFile::encodeName(KShell::quoteArg(d->mProcmailLockFileName));
      else
        cmd_str += QFile::encodeName(KShell::quoteArg(d->mMboxFile + ".lock"));

      rc = system(cmd_str.data());
      break;

    case mutt_dotlock:
      cmd_str = "mutt_dotlock -u " + QFile::encodeName(KShell::quoteArg(d->mMboxFile));
      rc = system(cmd_str.data());
      break;

    case mutt_dotlock_privileged:
      cmd_str = "mutt_dotlock -p -u " + QFile::encodeName(KShell::quoteArg(d->mMboxFile));
      rc = system(cmd_str.data());
      break;

    case lock_none: // Fall through.
    default:
      rc = 0;
      break;
  }
  return rc;
#endif
}
