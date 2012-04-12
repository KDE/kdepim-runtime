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

#include "setupdefaultfoldersjob.h"
#include "kolabdefs.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <collectionannotationsattribute.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectionmodifyjob.h>
#include <klocalizedstring.h>

using namespace Akonadi;

SetupDefaultFoldersJob::SetupDefaultFoldersJob(const Akonadi::AgentInstance& instance, QObject* parent): Job( parent ),
  m_agentInstance( instance )
{
  Q_ASSERT( instance.isValid() );
}

void SetupDefaultFoldersJob::doStart()
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  job->fetchScope().setResource( m_agentInstance.identifier() );
  connect( job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
}

void SetupDefaultFoldersJob::collectionFetchResult(KJob* job)
{
  if ( job->error() )
    return; // Akonadi::Job propagates that automatically

  // look for inbox
  Collection defaultParent;
  foreach ( const Collection &col, static_cast<CollectionFetchJob*>( job )->collections() ) {
    EntityDisplayAttribute* attr = 0;
    if ( (attr = col.attribute<EntityDisplayAttribute>()) ) {
      if ( attr->iconName() == QLatin1String( "mail-folder-inbox" ) ) {
        defaultParent = col;
      }
    }
  }

  if ( !defaultParent.isValid() ) {
    //If we can't find the parent, we end up creating toplevel directories (which is not what we want).
    kWarning() << "No inbox found!";
    setError( Job::Unknown );
    setErrorText( i18n( "No inbox found!" ) );
    emitResult();
    return;
  }

  // look for existing folders
  QVector<Collection> existingDefaultFolders( KolabV2::FolderTypeSize );
  QVector<Collection> recoveryCandidates( KolabV2::FolderTypeSize );
  foreach ( const Collection &col, static_cast<CollectionFetchJob*>( job )->collections() ) {
    if ( col.parentCollection() != defaultParent )
      continue;
    KolabV2::FolderType folderType = KolabV2::Mail;
    CollectionAnnotationsAttribute *attr = 0;
    if ( (attr = col.attribute<CollectionAnnotationsAttribute>()) ) {
      folderType = KolabV2::folderTypeFromString( attr->annotations().value( KOLAB_FOLDER_TYPE_ANNOTATION ) );
    }
    KolabV2::FolderType guessedType = KolabV2::guessFolderTypeFromName( col.name() );

    if ( folderType != KolabV2::Mail )
      existingDefaultFolders[ folderType ] = col;
    else if ( guessedType != KolabV2::Mail )
      recoveryCandidates[ guessedType ] = col;
  }

  // create/fix folders
  for ( int i = KolabV2::Contact; i < KolabV2::FolderTypeSize; ++i ) {
    QString iconName;
    if ( i == KolabV2::Mail )
    {
      //Nothing
    }
    else if ( i == KolabV2::Contact )
    {
      iconName = QString::fromLatin1( "view-pim-contacts" );
    }
    else if ( i == KolabV2::Event )
    {
      iconName = QString::fromLatin1( "view-calendar" );
    }
    else if ( i == KolabV2::Task )
    {
      iconName = QString::fromLatin1( "view-pim-tasks" );
    }
    else if ( i == KolabV2::Journal )
    {
      iconName = QString::fromLatin1( "view-pim-journal" );
    }
    else if ( i == KolabV2::Note )
    {
      iconName = QString::fromLatin1( "view-pim-notes" );
    }
    if ( existingDefaultFolders[ i ].isValid() ) {
      continue; // all good
    } else if ( recoveryCandidates[ i ].isValid() ) {
      Collection col = recoveryCandidates[ i ];
      CollectionAnnotationsAttribute* attr = col.attribute<CollectionAnnotationsAttribute>( Entity::AddIfMissing );
      QMap<QByteArray, QByteArray> annotations;
      annotations.insert( KOLAB_FOLDER_TYPE_ANNOTATION, KolabV2::folderTypeToString( static_cast<KolabV2::FolderType>( i ), true ) );
      attr->setAnnotations( annotations );
      if ( !iconName.isEmpty() ) {
        Akonadi::EntityDisplayAttribute *attribute =  col.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
        attribute->setIconName( iconName );
      }

      new CollectionModifyJob( col, 0 );
    } else {
      Collection col;
      col.setName( KolabV2::nameForFolderType( static_cast<KolabV2::FolderType>( i ) ) );
      col.setParentCollection( defaultParent );
      CollectionAnnotationsAttribute* attr = col.attribute<CollectionAnnotationsAttribute>( Entity::AddIfMissing );
      QMap<QByteArray, QByteArray> annotations;
      annotations.insert( KOLAB_FOLDER_TYPE_ANNOTATION, KolabV2::folderTypeToString( static_cast<KolabV2::FolderType>( i ), true ) );
      attr->setAnnotations( annotations );
      if ( !iconName.isEmpty() ) {
        Akonadi::EntityDisplayAttribute *attribute =  col.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
        attribute->setIconName( iconName );
      }
      new CollectionCreateJob( col, 0 );
    }
  }

  emitResult();
}

#include "setupdefaultfoldersjob.moc"
