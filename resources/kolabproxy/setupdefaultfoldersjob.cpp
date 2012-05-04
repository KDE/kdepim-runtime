/*
  Copyright (c) 2010 Volker Krause <vkrause@kde.org>
  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "setupdefaultfoldersjob.h"

#include "kolabdefs.h"

#include "collectionannotationsattribute.h" //from shared

#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>

SetupDefaultFoldersJob::SetupDefaultFoldersJob( const Akonadi::AgentInstance &instance,
                                                QObject *parent )
  : Job( parent ),
    m_agentInstance( instance )
{
  Q_ASSERT( instance.isValid() );
}

void SetupDefaultFoldersJob::doStart()
{
  Akonadi::CollectionFetchJob *job =
    new Akonadi::CollectionFetchJob( Akonadi::Collection::root(),
                                     Akonadi::CollectionFetchJob::Recursive, this );
  job->fetchScope().setResource( m_agentInstance.identifier() );
  connect( job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
}

void SetupDefaultFoldersJob::collectionFetchResult( KJob *job )
{
  if ( job->error() ) {
    return; // Akonadi::Job propagates that automatically
  }

  Akonadi::Collection::List collections;

  //FIXME: This should really look for the personal namespace, and use the folder there.
  //        As a workaround we try to use the inbox, and fallback to toplevel otherwise.
  // look for inbox
  Akonadi::Collection defaultParent;
  collections = static_cast<Akonadi::CollectionFetchJob*>( job )->collections();
  foreach ( const Akonadi::Collection &col, collections ) {
    if ( !( col.rights() & Akonadi::Collection::CanCreateCollection ) ) {
      continue;
    }
    Akonadi::EntityDisplayAttribute *attr = 0;
    if ( ( attr = col.attribute<Akonadi::EntityDisplayAttribute>() ) ) {
      if ( attr->iconName() == QLatin1String( "mail-folder-inbox" ) ) {
        defaultParent = col;
      }
    }
  }

  // look for existing folders
  QVector<Akonadi::Collection> existingDefaultFolders( KolabV2::FolderTypeSize );
  QVector<Akonadi::Collection> recoveryCandidates( KolabV2::FolderTypeSize );
  collections = static_cast<Akonadi::CollectionFetchJob*>( job )->collections();
  foreach ( const Akonadi::Collection &col, collections ) {
    if ( col.parentCollection() != defaultParent ) {
      continue;
    }
    KolabV2::FolderType folderType = KolabV2::Mail;
    Akonadi::CollectionAnnotationsAttribute *attr = 0;
    if ( ( attr = col.attribute<Akonadi::CollectionAnnotationsAttribute>() ) ) {
      folderType =
        KolabV2::folderTypeFromString(
          attr->annotations().value( KOLAB_FOLDER_TYPE_ANNOTATION ) );
    }
    KolabV2::FolderType guessedType = KolabV2::guessFolderTypeFromName( col.name() );

    if ( folderType != KolabV2::Mail ) {
      existingDefaultFolders[ folderType ] = col;
    } else if ( guessedType != KolabV2::Mail ) {
      recoveryCandidates[ guessedType ] = col;
    }
  }

  // create/fix folders
  for ( int i = KolabV2::Contact; i < KolabV2::FolderTypeSize; ++i ) {
    QString iconName;
    if ( i == KolabV2::Mail ) {
      //Nothing
    } else if ( i == KolabV2::Contact ) {
      iconName = QString::fromLatin1( "view-pim-contacts" );
    } else if ( i == KolabV2::Event ) {
      iconName = QString::fromLatin1( "view-calendar" );
    } else if ( i == KolabV2::Task ) {
      iconName = QString::fromLatin1( "view-pim-tasks" );
    } else if ( i == KolabV2::Journal ) {
      iconName = QString::fromLatin1( "view-pim-journal" );
    } else if ( i == KolabV2::Note ) {
      iconName = QString::fromLatin1( "view-pim-notes" );
    }

    if ( existingDefaultFolders[ i ].isValid() ) {
      continue; // all good
    } else if ( recoveryCandidates[ i ].isValid() ) {
      Akonadi::Collection col = recoveryCandidates[ i ];
      Akonadi::CollectionAnnotationsAttribute *attr =
        col.attribute<Akonadi::CollectionAnnotationsAttribute>( Akonadi::Entity::AddIfMissing );

      QMap<QByteArray, QByteArray> annotations;
      annotations.insert(
        KOLAB_FOLDER_TYPE_ANNOTATION,
        KolabV2::folderTypeToString( static_cast<KolabV2::FolderType>( i ), true ) );

      attr->setAnnotations( annotations );
      if ( !iconName.isEmpty() ) {
        Akonadi::EntityDisplayAttribute *attribute =
          col.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
        attribute->setIconName( iconName );
      }

      new Akonadi::CollectionModifyJob( col, 0 );
    } else {
      Akonadi::Collection col;
      col.setName( KolabV2::nameForFolderType( static_cast<KolabV2::FolderType>( i ) ) );
      col.setParentCollection( defaultParent );
      Akonadi::CollectionAnnotationsAttribute *attr =
        col.attribute<Akonadi::CollectionAnnotationsAttribute>( Akonadi::Entity::AddIfMissing );

      QMap<QByteArray, QByteArray> annotations;
      annotations.insert(
        KOLAB_FOLDER_TYPE_ANNOTATION,
        KolabV2::folderTypeToString( static_cast<KolabV2::FolderType>( i ), true ) );

      attr->setAnnotations( annotations );
      if ( !iconName.isEmpty() ) {
        Akonadi::EntityDisplayAttribute *attribute =
          col.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
        attribute->setIconName( iconName );
      }
      new Akonadi::CollectionCreateJob( col, 0 );
    }
  }

  emitResult();
}

#include "setupdefaultfoldersjob.moc"
