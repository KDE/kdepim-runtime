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

#include <klocale.h>

using namespace KPIM;

struct Maildir::Private
{
    Private( const QString& p )
    :path(p)
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


    QString path;
};

Maildir::Maildir( const QString& path )
:d( new Private(path) )
{
}

void Maildir::swap( const Maildir &rhs )
{
    Private *p = d;
    d = new Private( *rhs.d );
    delete p;
}


Maildir::Maildir(const Maildir & rhs)
  :d(0)
{
    swap(rhs);
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

  Q_FOREACH( QString p, paths )
  {
    if ( !QFile::exists(p) ) {
      error = i18n("Error opening %1; this folder is missing.");
      return false;
    }
    if ( !canAccess( p ) ) {
      error = i18n("Error opening %1; either this is not a valid "
                "maildir folder, or you do not have sufficient access permissions." );
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

QStringList Maildir::entryList() const
{
  QStringList result;
  if ( isValid() ) {
    result += d->listNew();
    result += d->listCurrent();
  }
//  kDebug() << "Maildir::entryList()" << result;
  return result;
}
