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

#include <kdebug.h>
#include <klocalizedstring.h>
#include <klockfile.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <QtCore/QFileInfo>
#include <QtCore/QReadWriteLock>
#include <QtCore/QProcess>

class MBox::Private
{
  public:
    Private(const QString &mboxFileName, bool readOnly)
      : mLock(mboxFileName)
      , mMboxFile(mboxFileName)
      , mReadOnly(readOnly)
    { }

    ~Private()
    {
      if (mMboxFile.isOpen())
        mMboxFile.close();
    }

    bool mFileLocked;
    KLockFile mLock;
    LockType mLockType;
    QFile mMboxFile;
    QString mProcmailLockFileName;
    bool mReadOnly;
};

/// private static methods.

QByteArray quoteAndEncode(const QString &str)
{
  return QFile::encodeName(KShell::quoteArg(str));
}

/// public methods.

MBox::MBox(const QString &mboxFile, bool readOnly)
  : d(new Private(mboxFile, readOnly))
{
  // Set some sane defaults
  d->mFileLocked = false;
  d->mLockType = KDELockFile;
}

MBox::~MBox()
{
  close();

  delete d;
}

void MBox::close()
{
  if (d->mFileLocked)
    unlock();

  if (d->mMboxFile.isOpen())
    d->mMboxFile.close();

  d->mFileLocked = false;
}

QList<MsgInfo> MBox::entryList(const QSet<int> &deletedItems) const
{
  Q_ASSERT(d->mMboxFile.isOpen());

  QRegExp regexp("^From .*[0-9][0-9]:[0-9][0-9]");
  QByteArray line;
  quint64 offs = 0; // The offset of the next message to read.

  QList<MsgInfo> result;

  while (!d->mMboxFile.atEnd()) {
    quint64 pos = d->mMboxFile.pos();

    line = d->mMboxFile.readLine();

    if (regexp.indexIn(line) >= 0 || d->mMboxFile.atEnd()) {
      // Found the seperator or at end of file, the message starts at offs
      quint64 msgSize = pos - offs;
      if(pos > 0 && !deletedItems.contains(offs)) {
        // This is not the seperator of the first mail in the file. If pos == 0
        // than we matched the separator of the first mail in the file.
        MsgInfo info;
        info.first = offs;
        info.second = msgSize;

        result << info;
        offs += msgSize; // Mark the beginning of the next message.
      }
    }
  }

  return result;
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
    errorMsg = i18n("%1 is not a file.").arg(info.absoluteFilePath());
    return false;
  }

  if (!info.exists()) {
    errorMsg = i18n("%1 does not exist").arg(info.absoluteFilePath());
    return false;
  }

  switch (d->mLockType) {
    case ProcmailLockfile:
      if (KStandardDirs::findExe("lockfile").isEmpty()) {
        errorMsg = i18n("Could not find the lockfile executable");
        return false;
      }
      break;
    case MuttDotlock: // fall through
    case MuttDotlockPrivileged:
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
  if (d->mMboxFile.isOpen() && openMode == Normal) {
    return 0;  // already open
  } else if (d->mMboxFile.isOpen()) {
    close(); // ReloadMode, so close the file first.
  }

  d->mFileLocked = false;

  if (int rc = lock()) {
    kDebug() << "Locking of the mbox file failed.";
    return rc;
  }

  if (!d->mMboxFile.open(QIODevice::ReadOnly)) { // messages file
    kDebug() << "Cannot open mbox file `" << d->mMboxFile.fileName() << "' FileError:"
             << d->mMboxFile.error();
    return d->mMboxFile.error();
  }

  return 0;
}

QByteArray MBox::readEntry(quint64 offset) const
{
  Q_ASSERT(d->mMboxFile.isOpen());
  Q_ASSERT(d->mMboxFile.size() > offset);

  d->mMboxFile.seek(offset);

  QByteArray line = d->mMboxFile.readLine();
  QRegExp regexp("^From .*[0-9][0-9]:[0-9][0-9]");
  if (regexp.indexIn(line) < 0)
    return QByteArray(); // The file is messed up or the index is incorrect.

  QByteArray message;
  message += line;
  line = d->mMboxFile.readLine();

  while (regexp.indexIn(line) < 0) {
    message += line;
    line = d->mMboxFile.readLine();
  }

  unescapeFrom(message.data(), message.size());

  return message;
}

QByteArray MBox::readEntryHeaders(quint64 offset)
{
  Q_ASSERT(d->mMboxFile.isOpen());
  Q_ASSERT(d->mMboxFile.size() > offset);

  d->mMboxFile.seek(offset);
  QByteArray headers;
  QByteArray line = d->mMboxFile.readLine();

  while (!line[0] == '\n') {
    headers += line;
    line = d->mMboxFile.readLine();
  }

  return headers;
}

void MBox::setLockType(LockType ltype)
{
  if (d->mFileLocked)
    return; // Don't change the method if the file is currently locked.

  d->mLockType = ltype;
}

void MBox::setProcmailLockFile(const QString &lockFile)
{
  d->mProcmailLockFileName = lockFile;
}

/// private methods

int MBox::lock()
{
  if (d->mLockType == None)
    return 0;

  d->mFileLocked = false;

  QStringList args;
  int rc = 0;

  switch(d->mLockType)
  {
    case KDELockFile:
      /* FIXME: Don't use the mbox file itself as lock file.
      if ((rc = d->mLock.lock(KLockFile::ForceFlag))) {
        kDebug() << "KLockFile lock failed: (" << rc
                 << ") switching to read only mode";
        d->mReadOnly = true;
      }
      */
      return 0;
      break; // We only need to lock the file using the QReadWriteLock

    case ProcmailLockfile:
      args << "-l20" << "-r5";
      if (!d->mProcmailLockFileName.isEmpty())
        args << quoteAndEncode(d->mProcmailLockFileName);
      else
        args << quoteAndEncode(d->mMboxFile.fileName() + ".lock");

      rc = QProcess::execute("lockfile", args);
      if(rc != 0) {
        kDebug() << "lockfile -l20 -r5 " << d->mMboxFile.fileName()
                 << ": Failed ("<< rc << ") switching to read only mode";
        d->mReadOnly = true; // In case the MBox object was created read/write we
                             // set it to read only when locking failed.
        return rc;
      }
      break;

    case MuttDotlock:
      args << quoteAndEncode(d->mMboxFile.fileName());
      rc = QProcess::execute("mutt_dotlock", args);

      if(rc != 0) {
        kDebug() << "mutt_dotlock " << d->mMboxFile.fileName()
                 << ": Failed (" << rc << ") switching to read only mode";
        d->mReadOnly = true; // In case the MBox object was created read/write we
                          // set it to read only when locking failed.
        return rc;
      }
      break;

    case MuttDotlockPrivileged:
      args << "-p" << quoteAndEncode(d->mMboxFile.fileName());
      rc = QProcess::execute("mutt_dotlock", args);

      if(rc != 0) {
        kDebug() << "mutt_dotlock -p " << d->mMboxFile.fileName() << ":"
                 << ": Failed (" << rc << ") switching to read only mode";
        d->mReadOnly = true;
        return rc;
      }
      break;

    case None: // This is never reached because of the check at the
      return 0;     // beginning of the function.
    default:
      break;
  }

  d->mFileLocked = true;
  return 0;
}

int MBox::unlock()
{
  int rc = 0;
  QStringList args;

  switch(d->mLockType)
  {
    case KDELockFile:
      // FIXME
      //d->mLock.unlock();
      break;

    case ProcmailLockfile:
      // QFile::remove returns true on succes so negate the result.
      if (!d->mProcmailLockFileName.isEmpty())
        rc = !QFile(d->mProcmailLockFileName).remove();
      else
        rc = !QFile(d->mMboxFile.fileName() + ".lock").remove();
      break;

    case MuttDotlock:
      args << "-u" << quoteAndEncode(d->mMboxFile.fileName());
      rc = QProcess::execute("mutt_dotlock", args);
      break;

    case MuttDotlockPrivileged:
      args << "-u" << "-p" << quoteAndEncode(d->mMboxFile.fileName());
      rc = QProcess::execute("mutt_dotlock", args);
      break;

    case None: // Fall through.
    default:
      break;
  }

  if (!rc) // Unlocking succeeded
    d->mFileLocked = false;

  return rc;
}

#define STRDIM(x) (sizeof(x)/sizeof(*x)-1)
// performs (\n|^)>{n}From_ -> \1>{n-1}From_ conversion
void MBox::unescapeFrom(char* str, size_t strLen)
{
  if (!str)
    return;
  if ( strLen <= STRDIM(">From ") )
    return;

  // yes, *d++ = *s++ is a no-op as long as d == s (until after the
  // first >From_), but writes are cheap compared to reads and the
  // data is already in the cache from the read, so special-casing
  // might even be slower...
  const char * s = str;
  char * d = str;
  const char * const e = str + strLen - STRDIM(">From ");

  while ( s < e ) {
    if ( *s == '\n' && *(s+1) == '>' ) { // we can do the lookahead, since e is 6 chars from the end!
      *d++ = *s++;  // == '\n'
      *d++ = *s++;  // == '>'
      while ( s < e && *s == '>' )
        *d++ = *s++;
      if ( qstrncmp( s, "From ", STRDIM("From ") ) == 0 )
        --d;
    }
    *d++ = *s++; // yes, s might be e here, but e is not the end :-)
  }
  // copy the rest:
  while ( s < str + strLen )
    *d++ = *s++;
  if ( d < s ) // only NUL-terminate if it's shorter
    *d = 0;
}
#undef STRDIM
