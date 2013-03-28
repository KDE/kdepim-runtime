/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#include "maildir.h"
#include "keycache.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHostInfo>
#include <QUuid>

#include <kdebug.h>
#include <klocale.h>
#include <kpimutils/kfileio.h>
#include <akonadi/kmime/messageflags.h>

#include <kde_file.h>
#include <time.h>
#include <unistd.h>

//Define it to get more debug output to expense of operating speed
// #define DEBUG_KEYCACHE_CONSITENCY


static void initRandomSeed()
{
  static bool init = false;
  if ( !init ) {
    unsigned int seed;
    init = true;
    int fd = KDE_open( "/dev/urandom", O_RDONLY );
    if ( fd < 0 || ::read( fd, &seed, sizeof( seed ) ) != sizeof( seed ) ) {
      // No /dev/urandom... try something else.
      srand( getpid() );
      seed = rand() + time( 0 );
    }

    if ( fd >= 0 )
      close( fd );

    qsrand( seed );
  }
}

using namespace KPIM;

Q_GLOBAL_STATIC_WITH_ARGS( QRegExp, statusSeparatorRx, (":|!") )

class Maildir::Private
{
public:
    Private( const QString& p, bool isRoot )
        :path(p), isRoot(isRoot)
    {
      hostName = QHostInfo::localHostName();
      // The default implementation of QUuid::createUuid() doesn't use
      // a seed that is random enough. Therefor we use our own initialization
      // until this issue will be fixed in Qt 4.7.
      initRandomSeed();

      //Cache object is created the first time this runs.
      //It will live throughout the lifetime of the application
      KeyCache::self()->addKeys( path );
    }

    Private( const Private& rhs )
    {
        path = rhs.path;
        isRoot = rhs.isRoot;
        hostName = rhs.hostName;
    }

    bool operator==( const Private& rhs ) const
    {
        return path == rhs.path;
    }
    bool accessIsPossible( QString& error, bool createMissingFolders = true ) const;
    bool canAccess( const QString& path ) const;

    QStringList subPaths() const
    {
        QStringList paths;
        paths << path + QString::fromLatin1( "/cur" );
        paths << path + QString::fromLatin1( "/new" );
        paths << path + QString::fromLatin1( "/tmp" );
        return paths;
    }

    QStringList listNew() const
    {
        QDir d( path + QString::fromLatin1( "/new" ) );
        return d.entryList( QDir::Files );
    }

    QStringList listCurrent() const
    {
        QDir d( path + QString::fromLatin1( "/cur" ) );
        return d.entryList( QDir::Files );
    }

    QString findRealKey( const QString& key ) const
    {
        KeyCache* keyCache = KeyCache::self();
        if ( keyCache->isNewKey( path, key ) ) {
#ifdef DEBUG_KEYCACHE_CONSITENCY
          if ( !QFile::exists( path + QString::fromLatin1( "/new/" ) + key ) ) {
            kDebug() << "WARNING: key is in cache, but the file is gone: " << path + QString::fromLatin1( "/new/" ) + key;
          }
#endif
          return path + QString::fromLatin1( "/new/" ) + key;
        }
        if ( keyCache->isCurKey( path, key ) ) {
#ifdef DEBUG_KEYCACHE_CONSITENCY
          if ( !QFile::exists( path + QString::fromLatin1( "/cur/" ) + key ) ) {
            kDebug() << "WARNING: key is in cache, but the file is gone: " << path + QString::fromLatin1( "/cur/" ) + key;
          }
#endif
          return path + QString::fromLatin1( "/cur/" ) + key;
        }
        QString realKey = path + QString::fromLatin1( "/new/" ) + key;

        QFile f( realKey );
        if ( f.exists() ) {
          keyCache->addNewKey( path, key );
        } else  { //not in "new", search in "cur"
          realKey = path + QString::fromLatin1( "/cur/" ) + key;
          QFile f2( realKey );
          if ( f2.exists() ) {
            keyCache->addCurKey( path, key );
          } else {
            realKey.clear(); //not in "cur" either
          }
        }

        return realKey;
    }

    static QString subDirNameForFolderName( const QString &folderName )
    {
        return QString::fromLatin1( ".%1.directory" ).arg( folderName );
    }

    QString subDirPath() const
    {
        QDir dir( path );
        return subDirNameForFolderName( dir.dirName() );
    }

    bool moveAndRename( QDir &dest, const QString &newName )
    {
      if ( !dest.exists() ) {
        kDebug() << "Destination does not exist";
        return false;
      }
      if ( dest.exists( newName ) || dest.exists( subDirNameForFolderName( newName ) ) ) {
        kDebug() << "New name already in use";
        return false;
      }

      if ( !dest.rename( path, newName ) ) {
        kDebug() << "Failed to rename maildir";
        return false;
      }
      const QDir subDirs( Maildir::subDirPathForFolderPath( path ) );
      if ( subDirs.exists() && !dest.rename( subDirs.path(), subDirNameForFolderName( newName ) ) ) {
        kDebug() << "Failed to rename subfolders";
        return false;
      }

      path = dest.path() + QDir::separator() + newName;
      return true;
    }

    QString path;
    bool isRoot;
    QString hostName;
};

Maildir::Maildir( const QString& path, bool isRoot )
:d( new Private(path, isRoot) )
{
}

void Maildir::swap( const Maildir &rhs )
{
    Private *p = d;
    d = new Private( *rhs.d );
    delete p;
}


Maildir::Maildir(const Maildir & rhs)
  :d( new Private( *rhs.d ) )

{
}

Maildir& Maildir::operator= (const Maildir & rhs)
{
    // copy and swap, exception safe, and handles assignment to self
    Maildir temp( rhs );
    swap( temp );
    return *this;
}


bool Maildir::operator== (const Maildir & rhs) const
{
    return *d == *rhs.d;
}


Maildir::~Maildir()
{
    delete d;
}

bool Maildir::Private::canAccess( const QString& path ) const
{
    //return access( QFile::encodeName( path ), R_OK | W_OK | X_OK ) != 0;
    // FIXME X_OK?
    QFileInfo d( path );
    return d.isReadable() && d.isWritable();
}

bool Maildir::Private::accessIsPossible( QString& error, bool createMissingFolders ) const
{
    QStringList paths = subPaths();
    
    paths.prepend( path );

    Q_FOREACH ( const QString &p, paths ) {
        if ( !QFile::exists( p ) ) {
            if ( !createMissingFolders ) {
              error = i18n( "Error opening %1; this folder is missing.", p );
              return false;
            }
            QDir().mkpath( p );
            if ( !QFile::exists( p ) ) {
              error = i18n( "Error opening %1; this folder is missing.", p );
              return false;
            }
        }
        if ( !canAccess( p ) ) {
            error = i18n( "Error opening %1; either this is not a valid "
                    "maildir folder, or you do not have sufficient access permissions." ,p );
            return false;
        }
    }
    return true;
}

bool Maildir::isValid( bool createMissingFolders ) const
{
    QString error;
    return isValid( error, createMissingFolders );
}

bool Maildir::isValid( QString &error, bool createMissingFolders ) const
{
    if ( !d->isRoot ) {
      if ( d->accessIsPossible( error, createMissingFolders ) ) {
          return true;
      }
    } else {
      Q_FOREACH ( const QString &sf, subFolderList() ) {
        const Maildir subMd = Maildir( path() + QLatin1Char( '/' ) + sf );
        if ( !subMd.isValid( error ) )
          return false;
      }
      return true;
    }
    return false;
}

bool Maildir::isRoot() const
{
  return d->isRoot;
}

bool Maildir::create()
{
    // FIXME: in a failure case, this will leave partially created dirs around
    // we should clean them up, but only if they didn't previously existed...
    Q_FOREACH ( const QString &p, d->subPaths() ) {
        QDir dir( p );
        if ( !dir.exists( p ) ) {
            if ( !dir.mkpath( p ) )
                return false;
        }
    }
    return true;
}

QString Maildir::path() const
{
    return d->path;
}

QString Maildir::name() const
{
  const QDir dir( d->path );
  return dir.dirName();
}

QString Maildir::addSubFolder( const QString& path )
{
    if ( !isValid() )
        return QString();

    // make the subdir dir
    QDir dir( d->path );
    if ( !d->isRoot ) {
        dir.cdUp();
        if ( !dir.exists( d->subDirPath() ) )
            dir.mkdir( d->subDirPath() );
        dir.cd( d->subDirPath() );
    }

    const QString fullPath = dir.path() + QLatin1Char( '/' ) + path;
    Maildir subdir( fullPath );
    if ( subdir.create() )
        return fullPath;
    return QString();
}

bool Maildir::removeSubFolder( const QString& folderName )
{
    if ( !isValid() ) return false;
    QDir dir( d->path );
    if ( !d->isRoot ) {
        dir.cdUp();
        if ( !dir.exists( d->subDirPath() ) ) return false;
        dir.cd( d->subDirPath() );
    }
    if ( !dir.exists( folderName ) ) return false;

    // remove it recursively
    bool result = KPIMUtils::removeDirAndContentsRecursively( dir.absolutePath() + QLatin1Char( '/' ) + folderName );
    QString subfolderName = subDirNameForFolderName(folderName);
    if ( dir.exists( subfolderName ) )
      result &= KPIMUtils::removeDirAndContentsRecursively( dir.absolutePath() + QLatin1Char( '/' ) + subfolderName );
    return result;
}

Maildir Maildir::subFolder( const QString& subFolder ) const
{
    // make the subdir dir
    QDir dir( d->path );
    if ( !d->isRoot ) {
        dir.cdUp();
        if ( dir.exists( d->subDirPath() ) ) {
            dir.cd( d->subDirPath() );
        }
    }
    return Maildir( dir.path() + QLatin1Char( '/' ) + subFolder );
}

Maildir Maildir::parent() const
{
  if ( !isValid() || d->isRoot )
    return Maildir();
  QDir dir( d->path );
  dir.cdUp();
  if ( !dir.dirName().startsWith( '.' ) || !dir.dirName().endsWith( QLatin1String( ".directory" ) ) )
    return Maildir();
  const QString parentName = dir.dirName().mid( 1, dir.dirName().size() - 11 );
  dir.cdUp();
  dir.cd( parentName );
  return Maildir ( dir.path() );
}

QStringList Maildir::entryList() const
{
    QStringList result;
    if ( isValid() ) {
        result += d->listNew();
        result += d->listCurrent();
    }
    //  kDebug() <<"Maildir::entryList()" << result;
    return result;
}

QStringList Maildir::listCurrent() const
{
    QStringList result;
    if ( isValid() ) {
        result += d->listCurrent();
    }
    return result;
}

QString Maildir::findRealKey(const QString& key) const
{
    return d->findRealKey( key );
}


QStringList Maildir::listNew() const
{
    QStringList result;
    if ( isValid() ) {
        result += d->listNew();
    }
    return result;
}

QString Maildir::subDirPath() const
{
  QDir dir( d->path );
  dir.cdUp();
  return dir.path() + QDir::separator() + d->subDirPath();
}



QStringList Maildir::subFolderList() const
{
    QDir dir( d->path );

    // the root maildir has its subfolders directly beneath it
    if ( !d->isRoot ) {
        dir.cdUp();
        if ( !dir.exists( d->subDirPath() ) ) return QStringList();
        dir.cd( d->subDirPath() );
    }
    dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
    QStringList entries = dir.entryList();
    entries.removeAll( QLatin1String( "cur" ) );
    entries.removeAll( QLatin1String( "new" ) );
    entries.removeAll( QLatin1String( "tmp" ) );
    return entries;
}

QByteArray Maildir::readEntry( const QString& key ) const
{
    QByteArray result;

    QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        // FIXME error handling?
        qWarning() << "Maildir::readEntry unable to find: " << key;
        return result;
    }

    QFile f( realKey );
    f.open( QIODevice::ReadOnly );

    // FIXME be safer than this
    result = f.readAll();

    return result;
}
qint64 Maildir::size( const QString& key ) const
{
    QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        // FIXME error handling?
        qWarning() << "Maildir::size unable to find: " << key;
        return -1;
    }

    QFileInfo info( realKey );
    if ( !info.exists() )
        return -1;

    return info.size();
}

QDateTime Maildir::lastModified(const QString& key) const
{
    const QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        qWarning() << "Maildir::lastModified unable to find: " << key;
        return QDateTime();
    }

    const QFileInfo info( realKey );
    if ( !info.exists() )
        return QDateTime();

    return info.lastModified();
}

QByteArray Maildir::readEntryHeadersFromFile( const QString& file ) const
{
    QByteArray result;

    QFile f( file );
    if ( !f.open( QIODevice::ReadOnly ) ) {
        // FIXME error handling?
        qWarning() << "Maildir::readEntryHeaders unable to find: " << file;
        return result;
    }
    f.map( 0, qMin( (qint64)8000, f.size() ) );
    forever {
        QByteArray line = f.readLine();
        if ( line.trimmed().isEmpty() )
            break;
        result.append( line );
    }
    return result;
}

QByteArray Maildir::readEntryHeaders( const QString& key ) const
{
    const QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        qWarning() << "Maildir::readEntryHeaders unable to find: " << key;
        return QByteArray();
    }

  return readEntryHeadersFromFile( realKey );
}


static QString createUniqueFileName()
{
    qint64 time = QDateTime::currentMSecsSinceEpoch() / 1000;
    int r = qrand() % 1000;
    QString identifier = QLatin1String( "R" ) + QString::number( r );

    QString fileName = QString::number( time ) + QLatin1String( "." ) + identifier + QLatin1String( "." );

    return fileName;
}

void Maildir::writeEntry( const QString& key, const QByteArray& data )
{
    QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        // FIXME error handling?
        qWarning() << "Maildir::writeEntry unable to find: " << key;
        return;
    }
    QFile f( realKey );
    f.open( QIODevice::WriteOnly );
    f.write( data );
    f.close();
}

QString Maildir::addEntry( const QByteArray& data )
{
    QString uniqueKey;
    QString key;
    QString finalKey;
    QString curKey;

    // QUuid doesn't return globally unique identifiers, therefor we query until we
    // get one that doesn't exists yet
    do {
      uniqueKey = createUniqueFileName() + d->hostName;
      key = d->path + QLatin1String( "/tmp/" ) + uniqueKey;
      finalKey = d->path + QLatin1String( "/new/" ) + uniqueKey;
      curKey = d->path + QLatin1String( "/cur/" ) + uniqueKey;
    } while ( QFile::exists( key ) || QFile::exists( finalKey ) || QFile::exists( curKey ) );

    QFile f( key );
    f.open( QIODevice::WriteOnly );
    f.write( data );
    f.close();
    /*
     * FIXME:
     *
     * The whole point of the locking free maildir idea is that the moves between
     * the internal directories are atomic. Afaik QFile::rename does not guarantee
     * that, so this will need to be done properly. - ta
     *
     * For reference: http://trolltech.com/developer/task-tracker/index_html?method=entry&id=211215
     */
    if ( !f.rename( finalKey ) ) {
        qWarning() << "Maildir: Failed to add entry: " << finalKey  << "! Error: " << f.errorString();
    }
    KeyCache *keyCache = KeyCache::self();
    keyCache->removeKey( d->path, key ); //remove all keys, be it "cur" or "new" first
    keyCache->addNewKey( d->path, key ); //and add a key for "new", as the mail was moved there
    return uniqueKey;
}

bool Maildir::removeEntry( const QString& key )
{
    QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        // FIXME error handling?
        qWarning() << "Maildir::removeEntry unable to find: " << key;
        return false;
    }
    KeyCache *keyCache = KeyCache::self();
    keyCache->removeKey( d->path, key );
    return QFile::remove( realKey );
}

QString Maildir::changeEntryFlags(const QString& key, const Akonadi::Item::Flags& flags)
{
    QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        // FIXME error handling?
        qWarning() << "Maildir::changeEntryFlags unable to find: " << key;
        return key;
    }

    const QRegExp rx = *( statusSeparatorRx() );
    QString finalKey = key.left( key.indexOf( rx ) );

    QStringList mailDirFlags;
    Q_FOREACH ( const Akonadi::Item::Flag flag, flags ) {
      if ( flag == Akonadi::MessageFlags::Forwarded )
        mailDirFlags << QLatin1String( "P" );
      if ( flag == Akonadi::MessageFlags::Replied )
        mailDirFlags << QLatin1String( "R" );
      if ( flag == Akonadi::MessageFlags::Seen )
        mailDirFlags << QLatin1String( "S" );
      if ( flag == Akonadi::MessageFlags::Deleted )
        mailDirFlags << QLatin1String( "T" );
      if ( flag == Akonadi::MessageFlags::Flagged )
        mailDirFlags << QLatin1String( "F" );
    }
    mailDirFlags.sort();
    if ( !mailDirFlags.isEmpty() ) {
#ifdef Q_OS_WIN
      finalKey.append( QLatin1String( "!2," ) + mailDirFlags.join( QString() ) );
#else
      finalKey.append( QLatin1String( ":2," ) + mailDirFlags.join( QString() ) );
#endif
    }

    QString newUniqueKey = finalKey; //key without path
    finalKey.prepend( d->path + QString::fromLatin1( "/cur/" ) );

    if ( realKey == finalKey ) {
      // Somehow it already is named this way (e.g. migration bug -> wrong status in akonadi)
      return newUniqueKey;
    }

    QFile f( realKey );
    if ( QFile::exists( finalKey ) ) {
      QFile destFile( finalKey );
      QByteArray destContent;
      if ( destFile.open( QIODevice::ReadOnly ) ) {
        destContent = destFile.readAll();
        destFile.close();
      }
      QByteArray sourceContent;
      if ( f.open( QIODevice::ReadOnly ) ) {
        sourceContent = f.readAll();
        f.close();
      }

      if ( destContent != sourceContent ) {
         QString newFinalKey = "1-" + newUniqueKey;
         int i = 1;
         while ( QFile::exists( d->path + QString::fromLatin1( "/cur/" ) + newFinalKey ) ) {
           i++;
           newFinalKey = QString::number( i ) + "-" + newUniqueKey;
         }
         finalKey = d->path + QString::fromLatin1( "/cur/" ) + newFinalKey;
      } else {
            QFile::remove( finalKey ); //they are the same
      }
    }

    if ( !f.rename( finalKey ) ) {
        qWarning() << "Maildir: Failed to rename entry: " << f.fileName() << " to "  << finalKey  << "! Error: " << f.errorString();
        return QString();
     }

    KeyCache *keyCache = KeyCache::self();
    keyCache->removeKey( d->path, key );
    keyCache->addCurKey( d->path, newUniqueKey );

    return newUniqueKey;
}

Akonadi::Item::Flags Maildir::readEntryFlags(const QString& key) const
{
    Akonadi::Item::Flags flags;

    const QRegExp rx = *( statusSeparatorRx() );
    const int index = key.indexOf( rx );
    if ( index != -1 ) {
        const QString mailDirFlags = key.mid( index + 3 ); // after "(:|!)2,"
        for ( int i = 0; i < mailDirFlags.size(); i++ ) {
            if ( mailDirFlags[i] == QLatin1Char( 'P' ) )
                flags << Akonadi::MessageFlags::Forwarded;
            else if ( mailDirFlags[i] == QLatin1Char( 'R' ) )
                flags << Akonadi::MessageFlags::Replied;
            else if ( mailDirFlags[i] == QLatin1Char( 'S' ) )
                flags << Akonadi::MessageFlags::Seen;
            else if ( mailDirFlags[i] == QLatin1Char( 'F' ) )
                flags << Akonadi::MessageFlags::Flagged;
        }
    }

    return flags;
}


bool Maildir::moveTo( const Maildir &newParent )
{
  if ( d->isRoot )
    return false; // not supported

  QDir newDir( newParent.path() );
  if ( !newParent.d->isRoot ) {
    newDir.cdUp();
    if ( !newDir.exists( newParent.d->subDirPath() ) )
      newDir.mkdir( newParent.d->subDirPath() );
    newDir.cd( newParent.d->subDirPath() );
  }

  QDir currentDir( d->path );
  currentDir.cdUp();

  if ( newDir == currentDir )
    return true;

  return d->moveAndRename( newDir, name() );
}

bool Maildir::rename( const QString &newName )
{
  if ( name() == newName )
    return true;
  if ( d->isRoot )
    return false; // not (yet) supported

  QDir dir( d->path );
  dir.cdUp();

  return d->moveAndRename( dir, newName );
}

QString Maildir::moveEntryTo( const QString &key, const Maildir &destination )
{
  const QString realKey( d->findRealKey( key ) );
  if ( realKey.isEmpty() ) {
    kDebug() << "Unable to find" << key;
    return QString();
  }
  QFile f( realKey );
  // ### is this safe regarding the maildir locking scheme?
  const QString targetKey = destination.path() + QDir::separator() + QLatin1String( "new" ) + QDir::separator() + key;
  if ( !f.rename( targetKey ) ) {
    kDebug() << "Failed to rename" << realKey << "to" << targetKey << "! Error: " << f.errorString();;
    return QString();
  }

  KeyCache* keyCache = KeyCache::self();

  keyCache->addNewKey( destination.path(), key );
  keyCache->removeKey( d->path, key );

  return key;
}

QString Maildir::subDirPathForFolderPath( const QString &folderPath )
{
  QDir dir( folderPath );
  const QString dirName = dir.dirName();
  dir.cdUp();
  return QFileInfo( dir, Private::subDirNameForFolderName( dirName ) ).filePath();
}

QString Maildir::subDirNameForFolderName( const QString &folderName )
{
  return Private::subDirNameForFolderName( folderName );
}

void Maildir::removeCachedKeys(const QStringList & keys)
{
  KeyCache *keyCache = KeyCache::self();
  Q_FOREACH ( const QString &key, keys ) {
    keyCache->removeKey( d->path, key );
  }
}

void Maildir::refreshKeyCache()
{
  KeyCache::self()->refreshKeys( d->path );
}
