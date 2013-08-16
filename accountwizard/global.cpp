/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "global.h"

#include <kdebug.h>
#include <KGlobal>
#include <KStandardDirs>
#include <QtCore/qfileinfo.h>
#include <QtCore/QDir>
#include <kio/copyjob.h>
#include <kio/netaccess.h>

class GlobalPrivate
{
  public:
    QStringList filter;
    QString assistant;
};

K_GLOBAL_STATIC( GlobalPrivate, sInstance )


QString Global::assistant()
{
  return sInstance->assistant;
}

void Global::setAssistant(const QString& assistant)
{
  const QFileInfo info( assistant );
  if ( info.isAbsolute() ) {
    sInstance->assistant = assistant;
    return;
  }

  const QStringList list = KGlobal::dirs()->findAllResources(
    "data", QLatin1String( "akonadi/accountwizard/*.desktop" ),
    KStandardDirs::Recursive | KStandardDirs::NoDuplicates );
  foreach ( const QString &entry, list ) {
    const QFileInfo info( entry );
    const QDir dir( info.absolutePath() );
    kDebug() << dir.dirName();
    if ( dir.dirName() == assistant ) {
      sInstance->assistant = entry;
      return;
    }
  }

  sInstance->assistant.clear();
}


QStringList Global::typeFilter()
{
  return sInstance->filter;
}

void Global::setTypeFilter(const QStringList& filter)
{
  sInstance->filter = filter;
}

QString Global::assistantBasePath()
{
  const QFileInfo info( assistant() );
  if ( info.isAbsolute() )
    return info.absolutePath() + QDir::separator();
  return QString();
}

QString Global::unpackAssistant( const KUrl& remotePackageUrl )
{
  QString localPackageFile;
  if ( remotePackageUrl.protocol() == QLatin1String( "file" ) ) {
    localPackageFile = remotePackageUrl.path();
  } else {
    QString remoteFileName = QFileInfo( remotePackageUrl.path() ).fileName();
    localPackageFile = KStandardDirs::locateLocal( "cache", QLatin1String("accountwizard/") + remoteFileName );
    KIO::Job* job = KIO::copy( remotePackageUrl, localPackageFile, KIO::Overwrite | KIO::HideProgressInfo );
    kDebug() << "downloading remote URL" << remotePackageUrl << "to" << localPackageFile;
    if ( !KIO::NetAccess::synchronousRun( job, 0 ) )
      return QString();
  }

  const KUrl file( QLatin1String("tar://") + localPackageFile );
  const QFileInfo fi( localPackageFile );
  const QString assistant = fi.baseName();
  const QString dest = KStandardDirs::locateLocal( "appdata", QLatin1String("/") );
  KStandardDirs::makeDir( dest + file.fileName() );
  KIO::Job* getJob = KIO::copy( file, dest, KIO::Overwrite | KIO::HideProgressInfo );
  if ( KIO::NetAccess::synchronousRun( getJob, 0 ) ) {
    kDebug() << "worked, unpacked in " << dest;
    return dest + file.fileName() + QLatin1Char('/') + assistant + QLatin1Char('/') + assistant + QLatin1String(".desktop");
  } else {
    kDebug() << "failed" << getJob->errorString();
    return QString();
  }
}
