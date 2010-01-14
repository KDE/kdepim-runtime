/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "mixedtreeconverter.h"

#include <libmaildir/maildir.h>
#include <KDebug>
#include <KLocalizedString>
#include <QtCore/QDir>

MixedTreeConverter::MixedTreeConverter(QObject* parent): QObject(parent)
{
}

void MixedTreeConverter::convert(const QString& basePath)
{
  const KPIM::Maildir md( basePath, true );
  if ( !md.isValid() ) {
    emit conversionDone( i18n( "'%1' is not a valid maildir folder.", basePath ) );
    return;
  }

  convert( md );

  emit conversionDone( QString() );
}

void MixedTreeConverter::convert(const KPIM::Maildir& md)
{
  kDebug() << md.path();
  if ( !md.isValid() )
    return;

  // recurse into maildirs, nothing to do there
  foreach ( const QString &subFolder, md.subFolderList() ) {
    kDebug() << subFolder;
    convert( md.subFolder( subFolder ) );
  }

  // code from Maildir::subFolder minus the safety checks
  QDir dir( md.path() );
  if ( !md.isRoot() ) {
    dir.cdUp();
    const QString subDirPath = QString::fromLatin1( ".%1.directory" ).arg( md.name() );
    if ( dir.exists( subDirPath ) ) {
      dir.cd( subDirPath );
    } else {
      return;
    }
  }

  // look for mboxes
  dir.setFilter( QDir::Files );
  foreach ( const QString &file, dir.entryList() ) {
    convertMbox( dir.absolutePath() + QDir::separator() + file );

    // recurse into mbox sub-folders
    QDir subDir( dir );
    const QString subDirPath = QString::fromLatin1( ".%1.directory" ).arg( file );
    if ( subDir.exists( subDirPath ) )
      subDir.cd( subDirPath );
    else
      continue;
    convert( KPIM::Maildir( subDir.path(), true ) );
  }
}

void MixedTreeConverter::convertMbox(const QString& path)
{
  kDebug() << path;
  emit conversionDone( i18n( "Mbox conversion not yet implemented: '%1'", path ) );
}


#include "mixedtreeconverter.moc"
