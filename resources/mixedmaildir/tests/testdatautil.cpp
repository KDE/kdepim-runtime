/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "testdatautil.h"

#include <KDebug>

#include <QDir>
#include <QFile>

using namespace TestDataUtil;

// use this instead of QFile::copy() because we want overwrite in case it exists
static bool copyFile( const QString &sourceFileName, const QString &targetFileName )
{
  QFile sourceFile( sourceFileName );
  QFile targetFile( targetFileName );

  if ( !sourceFile.open( QIODevice::ReadOnly ) ) {
    kError() << "Cannot open source file" << sourceFileName;
    return false;
  }

  if ( !targetFile.open( QIODevice::WriteOnly ) ) {
    kError() << "Cannot open target file" << targetFileName;
    return false;
  }

  return targetFile.write( sourceFile.readAll() ) != -1;
}

static bool copyFiles( const QDir &sourceDir, const QDir &targetDir )
{
  const QStringList files = sourceDir.entryList( QStringList(), QDir::Files );
  Q_FOREACH( const QString &file, files ) {
    const QFileInfo sourceFileInfo( sourceDir, file );
    const QFileInfo targetFileInfo( targetDir, file );
    if ( !copyFile( sourceFileInfo.absoluteFilePath(), targetFileInfo.absoluteFilePath() ) ) {
      kError() << "Failed to copy" << sourceFileInfo.absoluteFilePath()
               << "to" << targetFileInfo.absoluteFilePath();
      return false;
    }
  }

  return true;
}

FolderType TestDataUtil::folderType( const QString &testDataName )
{
  const QDir dir( QLatin1String( ":/data" ) );
  const QString indexFilePattern = QLatin1String( ".%1.index" );

  if ( !dir.exists( testDataName ) || !dir.exists( indexFilePattern.arg( testDataName ) ) ) {
    return InvalidFolder;
  }

  const QFileInfo fileInfo( dir, testDataName );
  return ( fileInfo.isDir() ? MaildirFolder : MBoxFolder );
}

QStringList TestDataUtil::testDataNames()
{
  const QDir dir( QLatin1String( ":/data" ) );
  const QFileInfoList dirEntries = dir.entryInfoList();

  const QString indexFilePattern = QLatin1String( ".%1.index" );

  QStringList result;
  Q_FOREACH( const QFileInfo& fileInfo, dirEntries ) {
    if ( dir.exists( indexFilePattern.arg( fileInfo.fileName() ) ) ) {
      result << fileInfo.fileName();
    }
  }

  result.sort();
  return result;
}

bool TestDataUtil::installFolder( const QString &testDataName, const QString &installPath, const QString &folderName )
{
  const FolderType type = TestDataUtil::folderType( testDataName );
  if ( type == InvalidFolder ) {
    kError() << "testDataName" << testDataName << "is not a valid mail folder type";
    return false;
  }

  if ( !QDir::current().mkpath( installPath ) ) {
    kError() << "Couldn't create installPath" << installPath;
    return false;
  }

  const QDir installDir( installPath );
  const QFileInfo installFileInfo( installDir, folderName );
  if ( installDir.exists( folderName ) ) {
    switch ( type ) {
      case MaildirFolder:
        if ( !installFileInfo.isDir() ) {
          kError() << "Target file name" << folderName << "already exists but is not a directory";
          return false;
        }
        break;

      case MBoxFolder:
        if ( !installFileInfo.isFile() ) {
          kError() << "Target file name" << folderName << "already exists but is not a directory";
          return false;
        }
        break;

      default:
        // already handled at beginning
        Q_ASSERT( false );
        return false;
    }
  }

  const QDir testDataDir( QLatin1String( ":/data" ) );

  switch ( type ) {
    case MaildirFolder: {
      const QString subPathPattern = QLatin1String( "%1/%2" );
      if ( !installDir.mkpath( subPathPattern.arg( folderName, QLatin1String( "new" ) ) ) ||
           !installDir.mkpath( subPathPattern.arg( folderName, QLatin1String( "cur" ) ) ) ||
           !installDir.mkpath( subPathPattern.arg( folderName, QLatin1String( "tmp" ) ) ) ) {
        kError() << "Couldn't create maildir directory structure";
        return false;
      }

      QDir sourceDir = testDataDir;
      QDir targetDir = installDir;

      sourceDir.cd( testDataName );
      targetDir.cd( folderName );

      if ( sourceDir.cd( QLatin1String( "new" ) ) ) {
        targetDir.cd( QLatin1String( "new" ) );
        if ( !copyFiles( sourceDir, targetDir ) ) {
          return false;
        }
        sourceDir.cdUp();
        targetDir.cdUp();
      }

      if ( sourceDir.cd( QLatin1String( "cur" ) ) ) {
        targetDir.cd( QLatin1String( "cur" ) );
        if ( !copyFiles( sourceDir, targetDir ) ) {
          return false;
        }
        sourceDir.cdUp();
        targetDir.cdUp();
      }

      if ( sourceDir.cd( QLatin1String( "tmp" ) ) ) {
        targetDir.cd( QLatin1String( "tmp" ) );
        if ( !copyFiles( sourceDir, targetDir ) ) {
          return false;
        }
      }
      break;
    }

    case MBoxFolder: {
      const QFileInfo mboxFileInfo( testDataDir, testDataName );
      if ( !copyFile( mboxFileInfo.absoluteFilePath(), installFileInfo.absoluteFilePath() ) ) {
        kError() << "Failed to copy" << mboxFileInfo.absoluteFilePath()
                 << "to" << installFileInfo.absoluteFilePath();
        return false;
      }
      break;
    }

    default:
      // already handled at beginning
      Q_ASSERT( false );
      return false;
  }

  const QString indexFilePattern = QLatin1String( ".%1.index" );
  const QFileInfo indexFileInfo( testDataDir, indexFilePattern.arg( testDataName ) );
  const QFileInfo indexInstallFileInfo( installDir, indexFilePattern.arg( folderName ) );

  return copyFile( indexFileInfo.absoluteFilePath(), indexInstallFileInfo.absoluteFilePath() );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
