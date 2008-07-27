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

#include <QDir>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>

#include <klocale.h>
#include <kpimutils/kfileio.h>

using namespace KPIM;

class Maildir::Private
{
public:
    Private( const QString& p )
    :path(p)
    {
    }

    Private( const QString& p, bool isRoot )
        :path(p), isRoot(isRoot)
    {
    }

    Private( const Private& rhs )
    {
        path = rhs.path;
    }

    bool operator==( const Private& rhs ) const
    {
        return path == rhs.path;
    }
    bool accessIsPossible( QString& error ) const;
    bool canAccess( const QString& path ) const;

    QStringList subPaths() const
    {
        QStringList paths;
        paths << path + QString::fromLatin1("/cur");
        paths << path + QString::fromLatin1("/new");
        paths << path + QString::fromLatin1("/tmp");
        return paths;
    }

    QStringList listNew() const
    {
        QDir d( path + QString::fromLatin1("/new") );
        return d.entryList(QDir::Files);
    }

    QStringList listCurrent() const
    {
        QDir d( path + QString::fromLatin1("/cur") );
        return d.entryList(QDir::Files);
    }

    QString findRealKey( const QString& key ) const
    {
        QString realKey = path + QString::fromLatin1("/new/") + key;
        QFile f( realKey );
        if ( !f.exists() )
            realKey = path + QString::fromLatin1("/cur/") + key;
        QFile f2( realKey );
        if ( !f2.exists() )
            realKey.clear();
        return realKey;
    }

    QString subDirPath() const
    {
        QDir dir( path );
        return QString::fromLatin1(".%1.directory").arg( dir.dirName() );
    }

    QString path;
    bool isRoot;
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
  :d( new Private(*rhs.d) )

{
}


Maildir& Maildir::operator= (const Maildir & rhs)
{
    // copy and swap, exception safe, and handles assignment to self
    Maildir temp(rhs);
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
    //return access(QFile::encodeName( path ), R_OK | W_OK | X_OK ) != 0;
    // FIXME X_OK?
    QFileInfo d(path);
    return d.isReadable() && d.isWritable();
}

bool Maildir::Private::accessIsPossible( QString& error ) const
{
    QStringList paths = subPaths();
    paths.prepend( path );

    Q_FOREACH( const QString &p, paths )
    {
        if ( !QFile::exists(p) ) {
            error = i18n("Error opening %1; this folder is missing.",p);
            return false;
        }
        if ( !canAccess( p ) ) {
            error = i18n("Error opening %1; either this is not a valid "
                    "maildir folder, or you do not have sufficient access permissions." ,p);
            return false;
        }
    }
    return true;
}

bool Maildir::isValid() const
{
    QString error;
    return isValid( error );
}

bool Maildir::isValid( QString &error ) const
{
    if ( d->accessIsPossible( error ) ) {
        return true;
    }
    return false;
}


bool Maildir::create()
{
    // FIXME: in a failure case, this will leave partially created dirs around
    // we should clean them up, but only if they didn't previously existed...
    Q_FOREACH( const QString &p, d->subPaths() ) {
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

bool Maildir::addSubFolder( const QString& path )
{
    if ( !isValid() ) return false;
    // make the subdir dir
    QDir dir( d->path );
    if ( !d->isRoot ) {
        dir.cdUp();
        if (!dir.exists( d->subDirPath() ) )
            dir.mkdir( d->subDirPath() );
        dir.cd( d->subDirPath() );
    }

    const QString fullPath = dir.path() + '/' + path;
    Maildir subdir( fullPath );
    if ( subdir.create() )
        return true;
    return false;
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
    return KPIMUtils::removeDirAndContentsRecursively( dir.absolutePath() + '/' + folderName );
}

Maildir Maildir::subFolder( const QString& subFolder )
{
    if ( isValid() ) {
        // make the subdir dir
        QDir dir( d->path );
        if ( !d->isRoot ) {
            dir.cdUp();
            if ( dir.exists( d->subDirPath() ) ) {
                dir.cd( d->subDirPath() );
            }
        }
        return Maildir( dir.path() + '/' + subFolder );
    }
    return Maildir();
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


QStringList Maildir::subFolderList() const
{
    QDir dir( d->path );

    // the root maildir has its subfolders directly beneath it
    if ( !d->isRoot ) {
        dir.cdUp();
        if (!dir.exists( d->subDirPath() ) ) return QStringList();
        dir.cd( d->subDirPath() );
    }
    dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
    QStringList entries = dir.entryList();
    entries.removeAll( "cur" );
    entries.removeAll( "new" );
    entries.removeAll( "tmp" );
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

QByteArray Maildir::readEntryHeaders( const QString& key ) const
{
    QByteArray result;

    QString realKey( d->findRealKey( key ) );
    if ( realKey.isEmpty() ) {
        // FIXME error handling?
        qWarning() << "Maildir::readEntryHeaders unable to find: " << key;
        return result;
    }

    QFile f( realKey );
    if ( !f.open( QIODevice::ReadOnly ) ) {
        // FIXME error handling?
        qWarning() << "Maildir::readEntryHeaders unable to find: " << key;
        return result;
    }
    forever {
        QByteArray line = f.readLine();
        if ( line.trimmed().isEmpty() )
            break;
        result.append( line );
    }
    return result;
}


static QString createUniqueFileName()
{
    return QUuid::createUuid().toString();
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
    QString uniqueKey( createUniqueFileName() );
    QString key( d->path + "/tmp/" + uniqueKey );
    QString finalKey( d->path + "/new/" + uniqueKey );
    QFile f( key );
    f.open( QIODevice::WriteOnly );
    f.write( data );
    f.close();
    /*
     * FIXME:
     *
     * THe whole point of the locking free maildir idea is that the moves between
     * the internal directories are atomic. Afaik QFile::rename does not guarantee
     * that, so this will need to be done properly. - ta
     */
    if (!f.rename( finalKey )) {
        qWarning() << "Maildir: Failed to add entry: " << finalKey  << "!";
    }
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
    return QFile::remove(realKey);
}


