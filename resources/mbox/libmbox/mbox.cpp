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
          kdepim/kmail/kmfoldermbox.cpp. This is why I added a line for Stefan
          Taferner.

          Bertjan Broeksema, april 2009
*/

#include "mbox.h"

#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QReadWriteLock>

#include <fcntl.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kurl.h>

class MBox::Private
{
  public:
    Private() : mInitialMboxFileSize( 0 )
    {}

    ~Private()
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

    QByteArray mAppendedEntries;
    QList<MsgInfo> mEntries;
    bool mFileLocked;
    quint64 mInitialMboxFileSize;
    LockType mLockType;
    QFile mMboxFile;
    QString mLockFileName;
    bool mReadOnly;
};

static QString sMBoxSeperatorRegExp( "^From .*[0-9][0-9]:[0-9][0-9]" );

/// private static methods.

QByteArray quoteAndEncode( const QString &str )
{
  return QFile::encodeName( KShell::quoteArg( str ) );
}

/// public methods.

MBox::MBox()
  : d( new Private() )
{
  // Set some sane defaults
  d->mFileLocked = false;
  d->mLockType = None; //
}

MBox::~MBox()
{
  if ( d->mFileLocked )
    unlock();

  d->close();

  delete d;
}

qint64 MBox::appendEntry( const MessagePtr &entry )
{
  if ( d->mMboxFile.fileName().isEmpty() )
    return -1; // It doesn't make sense to add entries when we don't have an reference file.

  const QByteArray rawEntry = escapeFrom( entry->encodedContent() );

  if ( rawEntry.size() <= 0 ) {
    kDebug() << "Message added to folder `" << d->mMboxFile.fileName()
             << "' contains no data. Ignoring it.";
    return -1;
  }

  int nextOffset = d->mAppendedEntries.size(); // Offset of the appended message

  // Make sure the byte array is large enough to check for an end character.
  // Then check if the required newlines are there.
  if ( nextOffset < 1 && d->mMboxFile.size() > 0 ) { // Empty, add one empty line
    d->mAppendedEntries.append( "\n");
    ++nextOffset;
  } else if ( nextOffset == 1 && d->mAppendedEntries.at( 0 ) != '\n' ) {
    // This should actually not happen, but catch it anyway.
    if ( d->mMboxFile.size() < 0 ) {
      d->mAppendedEntries.append( "\n" );
      ++nextOffset;
    }
  } else if ( nextOffset >= 2 ) {
    if ( d->mAppendedEntries.at( nextOffset - 1 ) != '\n' ) {
      if ( d->mAppendedEntries.at( nextOffset ) != '\n' ) {
        d->mAppendedEntries.append( "\n\n" );
        nextOffset += 2;
      } else {
        d->mAppendedEntries.append( "\n" );
        ++nextOffset;
      }
    }
  }

  d->mAppendedEntries.append( mboxMessageSeparator( rawEntry ) );
  d->mAppendedEntries.append( rawEntry );
  if ( rawEntry[rawEntry.size() - 1] != '\n' ) {
    d->mAppendedEntries.append( "\n\n" );
  } else {
    d->mAppendedEntries.append( "\n" );
  }

  MsgInfo info;
  info.first = d->mInitialMboxFileSize + nextOffset;
  info.second = rawEntry.size();
  d->mEntries << info;

  return d->mInitialMboxFileSize + nextOffset;
}

QList<MsgInfo> MBox::entryList(const QSet<quint64> &deletedItems) const
{
  QList<MsgInfo> result;

  foreach ( const MsgInfo &info, d->mEntries ) {
    if ( !deletedItems.contains( info.first ) )
      result << info;
  }

  return result;
}

bool MBox::load( const QString &fileName )
{
  if ( d->mFileLocked )
    return false;

  d->mMboxFile.setFileName( KUrl( fileName ).path() );
  if ( !d->mMboxFile.exists() && !d->mMboxFile.open( QIODevice::WriteOnly ) )
    return false;

  if ( !lock() )
    return false;

  d->mInitialMboxFileSize = d->mMboxFile.size();
  d->mAppendedEntries.clear();
  d->mEntries.clear();

  QRegExp regexp( sMBoxSeperatorRegExp );
  QByteArray line;
  QByteArray prevSeparator;
  quint64 offs = 0; // The offset of the next message to read.

  while ( !d->mMboxFile.atEnd() ) {
    quint64 pos = d->mMboxFile.pos();

    line = d->mMboxFile.readLine();

    if ( regexp.indexIn( line ) >= 0 || d->mMboxFile.atEnd() ) {
      // Found the separator or at end of file, the message starts at offs
      quint64 msgSize = pos - offs;

      if( pos > 0 ) {
        // This is not the separator of the first mail in the file. If pos == 0
        // than we matched the separator of the first mail in the file.
        MsgInfo info;
        info.first = offs;

        // There is always a blank line and a separator line between two emails.
        // Sometimes there are two '\n' characters added to the email (i.e. when
        // the mail self did not end with a '\n' char) and sometimes only one to
        // achieve this. When reading the file it is not possible to see which
        // was the case.
        if ( d->mMboxFile.atEnd() )
          info.second = msgSize; // We use readLine so there's no additional '\n'
        else
          info.second = msgSize - 1;

        // Don't add the separator size and the newline up to the message size.
        info.second -= prevSeparator.size() + 1;

        d->mEntries << info;
      }

      if ( regexp.indexIn( line ) >= 0 )
        prevSeparator = line;

      offs += msgSize; // Mark the beginning of the next message.
    }
  }

  return unlock(); // FIXME: What if unlock fails?
}

bool MBox::lock()
{
  if ( d->mMboxFile.fileName().isEmpty() )
    return false; // We cannot lock if there is no file loaded.

  d->mFileLocked = false;

  QStringList args;
  int rc = 0;

  switch(d->mLockType)
  {
    case ProcmailLockfile:
      args << "-l20" << "-r5";
      if ( !d->mLockFileName.isEmpty() )
        args << quoteAndEncode(d->mLockFileName);
      else
        args << quoteAndEncode(d->mMboxFile.fileName() + ".lock");

      rc = QProcess::execute("lockfile", args);
      if( rc != 0 ) {
        kDebug() << "lockfile -l20 -r5 " << d->mMboxFile.fileName()
                 << ": Failed ("<< rc << ") switching to read only mode";
        d->mReadOnly = true; // In case the MBox object was created read/write we
                             // set it to read only when locking failed.
      } else {
        d->mFileLocked = true;
      }
      break;

    case MuttDotlock:
      args << quoteAndEncode( d->mMboxFile.fileName() );
      rc = QProcess::execute( "mutt_dotlock", args );

      if( rc != 0 ) {
        kDebug() << "mutt_dotlock " << d->mMboxFile.fileName()
                 << ": Failed (" << rc << ") switching to read only mode";
        d->mReadOnly = true; // In case the MBox object was created read/write we
                          // set it to read only when locking failed.
      } else {
        d->mFileLocked = true;
      }
      break;

    case MuttDotlockPrivileged:
      args << "-p" << quoteAndEncode( d->mMboxFile.fileName() );
      rc = QProcess::execute( "mutt_dotlock", args );

      if( rc != 0 ) {
        kDebug() << "mutt_dotlock -p " << d->mMboxFile.fileName() << ":"
                 << ": Failed (" << rc << ") switching to read only mode";
        d->mReadOnly = true;
      } else {
        d->mFileLocked = true;
      }
      break;

    case None:
      d->mFileLocked = true;
      break;
    default:
      break;
  }

  if ( d->mFileLocked ) {
    if ( !open() ) {
      const bool unlocked = unlock();
      Q_ASSERT( unlocked ); // If this fails we're in trouble.
      Q_UNUSED( unlocked );
    }
  }

  return d->mFileLocked;
}

static bool lessThanByOffset( const MsgInfo &left, const MsgInfo &right )
{
  return left.first < right.first;
}

bool MBox::purge( const QSet<quint64> &deletedItems )
{
  if ( d->mMboxFile.fileName().isEmpty() )
    return false; // No file loaded yet.

  if ( deletedItems.isEmpty() )
    return true; // Nothing to do.

  if ( !lock() )
    return false;

  foreach ( quint64 offset, deletedItems ) {
    d->mMboxFile.seek( offset );
    QByteArray line = d->mMboxFile.readLine();
    QRegExp regexp( sMBoxSeperatorRegExp );

    if ( regexp.indexIn(line) < 0 ) {
      qDebug() << "Found invalid separator at:" << offset;
      unlock();
      return false; // The file is messed up or the index is incorrect.
    }
  }

  // All entries are deleted, so just resize the file to a size of 0.
  if ( deletedItems.size() == d->mEntries.size() ) {
    d->mEntries.clear();
    d->mMboxFile.resize( 0 );
    kDebug() << "Purge comleted successfully, unlocking the file.";
    return unlock();
  }

  qSort( d->mEntries.begin(), d->mEntries.end(), lessThanByOffset );
  quint64 writeOffset = 0;
  bool writeOffSetInitialized = false;
  QList<MsgInfo> resultingEntryList;

  quint64 origFileSize = d->mMboxFile.size();

  qDebug() << "ENTRIES:" << d->mEntries;
  QListIterator<MsgInfo> i( d->mEntries );
  while ( i.hasNext() ) {
    MsgInfo entry = i.next();

    if ( deletedItems.contains( entry.first ) && !writeOffSetInitialized ) {
      writeOffset = entry.first;
      writeOffSetInitialized = true;
    } else if ( writeOffSetInitialized
                && writeOffset < entry.first
                && !deletedItems.contains( entry.first ) ) {
      // The current message doesn't have to be deleted, but must be moved.
      // First determine the size of the entry that must be moved.
      quint64 entrySize = 0;
      if ( i.hasNext() ) {
        entrySize = i.next().first - entry.first;
        i.previous(); // Go back to make sure that we also handle the next entry.
      } else {
        entrySize = origFileSize - entry.first;
      }

      Q_ASSERT( entrySize > 0 ); // MBox entries really cannot have a size <= 0;

      // we map the whole area of the file starting at the writeOffset up to the
      // message that have to be moved into memory. This includes eventually the
      // messages that are the deleted between the first deleted message
      // encountered and the message that has to be moved.
      quint64 mapSize = entry.first + entrySize - writeOffset;

      // Now map writeOffSet + mapSize into mem.
      uchar *memArea = d->mMboxFile.map( writeOffset, mapSize );

      // Now read the entry that must be moved to writeOffset.
      quint64 startOffset = entry.first - writeOffset;
      memmove( memArea, memArea + startOffset, entrySize );

      d->mMboxFile.unmap( memArea );

      resultingEntryList << MsgInfo( writeOffset, entry.second );
      writeOffset += entrySize;
    } else if ( !deletedItems.contains( entry.first ) ) {
      // Unmoved and not deleted entry, can only ocure before the first deleted
      // entry.
      Q_ASSERT( !writeOffSetInitialized );
      resultingEntryList << entry;
    }
  }

  // Chop off remaining entry bits.
  d->mMboxFile.resize( writeOffset );
  d->mEntries = resultingEntryList;

  kDebug() << "Purge comleted successfully, unlocking the file.";
  return unlock(); // FIXME: What if this fails? It will return false but the
                   // file has changed.
}

KMime::Message *MBox::readEntry(quint64 offset)
{
  bool wasLocked = d->mFileLocked;
  if ( ! wasLocked )
    if ( ! lock() )
      return 0;

  // TODO: Add error handling in case locking failed.

  Q_ASSERT( d->mFileLocked );
  Q_ASSERT( d->mMboxFile.isOpen() );
  Q_ASSERT( d->mMboxFile.size() > 0 );

  if ( offset > static_cast<quint64>( d->mMboxFile.size() ) ) {
    unlock();
    return 0;
  }

  d->mMboxFile.seek(offset);

  QByteArray line = d->mMboxFile.readLine();
  QRegExp regexp( sMBoxSeperatorRegExp );

  if ( regexp.indexIn( line ) < 0) {
    unlock();
    return 0; // The file is messed up or the index is incorrect.
  }

  QByteArray message;
  line = d->mMboxFile.readLine();
  while ( regexp.indexIn( line ) < 0 && !d->mMboxFile.atEnd() ) {
    message += line;
    line = d->mMboxFile.readLine();
  }

  // Remove te last '\n' added by writeEntry.
  if ( message.endsWith( '\n' ) )
    message.chop(1);

  unescapeFrom( message.data(), message.size() );

  if ( ! wasLocked ) {
    const bool unlocked = unlock();
    Q_ASSERT( unlocked );
    Q_UNUSED( unlocked );
  }

  KMime::Message *mail = new KMime::Message();
  mail->setContent( KMime::CRLFtoLF( message ) );
  mail->parse();

  return mail;
}

QByteArray MBox::readEntryHeaders( quint64 offset )
{
  bool wasLocked = d->mFileLocked;
  if ( ! wasLocked )
    lock();

  Q_ASSERT( d->mFileLocked );
  Q_ASSERT( d->mMboxFile.isOpen() );
  Q_ASSERT( d->mMboxFile.size() > 0 );
  Q_ASSERT( static_cast<quint64>(d->mMboxFile.size()) > offset );

  d->mMboxFile.seek( offset );
  QByteArray headers;
  QByteArray line = d->mMboxFile.readLine();

  while ( !line[0] == '\n' ) {
    headers += line;
    line = d->mMboxFile.readLine();
  }

  if ( ! wasLocked )
    unlock();

  return headers;
}

bool MBox::save( const QString &fileName )
{
  if ( !fileName.isEmpty() && KUrl( fileName ).path() != d->mMboxFile.fileName() )
  {
    // File saved != file loaded from
    return false; // FIXME: Implement this case
  }

  if ( d->mAppendedEntries.size() == 0 )
    return true; // Nothing to do.

  if ( !lock() )
    return false;

  Q_ASSERT( d->mMboxFile.isOpen() );

  d->mMboxFile.seek( d->mMboxFile.size() );
  d->mMboxFile.write( d->mAppendedEntries );
  d->mAppendedEntries.clear();

  return unlock();
}

bool MBox::setLockType(LockType ltype)
{
  if (d->mFileLocked) {
    kDebug() << "File is currently locked.";
    return false; // Don't change the method if the file is currently locked.
  }

  switch ( ltype ) {
    case ProcmailLockfile:
      if ( KStandardDirs::findExe( "lockfile" ).isEmpty() ) {
        kDebug() << "Could not find the lockfile executable";
        return false;
      }
      break;
    case MuttDotlock: // fall through
    case MuttDotlockPrivileged:
      if ( KStandardDirs::findExe("mutt_dotlock").isEmpty() ) {
        kDebug() << "Could not find the mutt_dotlock executable";
        return false;
      }
      break;
    default:
      break; // We assume fcntl available and lock_none doesn't need a check.
  }

  d->mLockType = ltype;
  return true;
}

void MBox::setLockFile( const QString &lockFile )
{
  d->mLockFileName = lockFile;
}

bool MBox::unlock()
{
  int rc = 0;
  QStringList args;

  switch( d->mLockType )
  {
    case ProcmailLockfile:
      // QFile::remove returns true on succes so negate the result.
      if ( !d->mLockFileName.isEmpty() )
        rc = !QFile( d->mLockFileName ).remove();
      else
        rc = !QFile( d->mMboxFile.fileName() + ".lock" ).remove();
      break;

    case MuttDotlock:
      args << "-u" << quoteAndEncode( d->mMboxFile.fileName() );
      rc = QProcess::execute( "mutt_dotlock", args );
      break;

    case MuttDotlockPrivileged:
      args << "-u" << "-p" << quoteAndEncode( d->mMboxFile.fileName() );
      rc = QProcess::execute( "mutt_dotlock", args );
      break;

    case None: // Fall through.
    default:
      break;
  }

  if ( rc == 0 ) // Unlocking succeeded
    d->mFileLocked = false;

  d->mMboxFile.close();

  return !d->mFileLocked;
}

/// private methods

bool MBox::open()
{
  if ( d->mMboxFile.isOpen() )
    return true;  // already open

  if ( !d->mMboxFile.open( QIODevice::ReadWrite ) ) { // messages file
    kDebug() << "Cannot open mbox file `" << d->mMboxFile.fileName() << "' FileError:"
             << d->mMboxFile.error();
    return false;
  }

  return true;
}

QByteArray MBox::mboxMessageSeparator( const QByteArray &msg )
{
  KMime::Message mail;
  mail.setHead( KMime::CRLFtoLF( msg ) );
  mail.parse();

  QByteArray separator = "From ";

  KMime::Headers::From *from = mail.from( false );
  if ( !from || from->addresses().isEmpty() )
    separator += "unknown@unknown.invalid";
  else
    separator += from->addresses().first() + ' ';

  KMime::Headers::Date *date = mail.date(false);
  if (!date || date->isEmpty())
    separator += QDateTime::currentDateTime().toString( Qt::TextDate ).toUtf8() + '\n';
  else
    separator += date->as7BitString(false) + '\n';

  return separator;
}

#define STRDIM(x) (sizeof(x)/sizeof(*x)-1)

QByteArray MBox::escapeFrom( const QByteArray &str )
{
  const unsigned int strLen = str.length();
  if ( strLen <= STRDIM( "From " ) )
    return str;

  // worst case: \nFrom_\nFrom_\nFrom_... => grows to 7/6
  QByteArray result( int( strLen + 5 ) / 6 * 7 + 1, '\0');

  const char * s = str.data();
  const char * const e = s + strLen - STRDIM( "From ");
  char * d = result.data();

  bool onlyAnglesAfterLF = false; // dont' match ^From_
  while ( s < e ) {
    switch ( *s ) {
    case '\n':
      onlyAnglesAfterLF = true;
      break;
    case '>':
      break;
    case 'F':
      if ( onlyAnglesAfterLF && qstrncmp( s+1, "rom ", STRDIM("rom ") ) == 0 )
        *d++ = '>';
      // fall through
    default:
      onlyAnglesAfterLF = false;
      break;
    }
    *d++ = *s++;
  }
  while ( s < str.data() + strLen )
    *d++ = *s++;

  result.truncate( d - result.data() );
  return result;
}

// performs (\n|^)>{n}From_ -> \1>{n-1}From_ conversion
void MBox::unescapeFrom( char* str, size_t strLen )
{
  if ( !str )
    return;
  if ( strLen <= STRDIM(">From ") )
    return;

  // yes, *d++ = *s++ is a no-op as long as d == s (until after the
  // first >From_), but writes are cheap compared to reads and the
  // data is already in the cache from the read, so special-casing
  // might even be slower...
  const char * s = str;
  char * d = str;
  const char * const e = str + strLen - STRDIM( ">From ");

  while ( s < e ) {
    if ( *s == '\n' && *(s+1) == '>' ) { // we can do the lookahead, since e is 6 chars from the end!
      *d++ = *s++;  // == '\n'
      *d++ = *s++;  // == '>'
      while ( s < e && *s == '>' )
        *d++ = *s++;
      if ( qstrncmp( s, "From ", STRDIM( "From ") ) == 0 )
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
