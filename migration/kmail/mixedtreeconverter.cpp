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
#include <libmbox/mbox.h>
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

  // (0) check if this mbox has been converted before already
  if ( path.endsWith( ".kmail-migrator-backup" ) )
    return;

  // (1) move mbox out of the way to make room for the maildir
  const QString mboxBackupPath( path + ".kmail-migrator-backup" );
  if ( !QFile::rename( path, mboxBackupPath ) ) {
    emit conversionDone( i18n( "Unable to move mbox file '%1' to backup location.", path ) );
    return;
  }

  // (2) create the maildir that will contain the mbox content
  KPIM::Maildir md( path );
  if ( !md.create() ) {
    emit conversionDone( i18n( "Unable to create maildir folder at '%1'.", path ) );
    return;
  }

  // (3) move messages from mbox to maildir
  MBox mbox;
  if ( !mbox.load( mboxBackupPath ) ) {
    emit conversionDone( i18n( "Unable to open mbox file at '%1'.", mboxBackupPath ) );
    return;
  }
  foreach ( const MsgInfo &entry, mbox.entryList() ) {
    if ( md.addEntry( mbox.readRawEntry( entry.first ) ).isEmpty() ) {
      emit conversionDone( i18n( "Unable to add new message to maildir '%1'.", path ) );
      return;
    }
  }

  emit conversionDone( "bla" );
}


#include "mixedtreeconverter.moc"
