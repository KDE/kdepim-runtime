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
    // TODO: implement me
    kWarning() << "No inbox found!";
  }

  // look for existing folders
  QVector<Collection> existingDefaultFolders( Kolab::FolderTypeSize );
  QVector<Collection> recoveryCandidates( Kolab::FolderTypeSize );
  foreach ( const Collection &col, static_cast<CollectionFetchJob*>( job )->collections() ) {
    if ( col.parentCollection() != defaultParent )
      continue;
    Kolab::FolderType folderType = Kolab::Mail;
    CollectionAnnotationsAttribute *attr = 0;
    if ( (attr = col.attribute<CollectionAnnotationsAttribute>()) ) {
      folderType = Kolab::folderTypeFromString( attr->annotations().value( KOLAB_FOLDER_TYPE_ANNOTATION ) );
    }
    Kolab::FolderType guessedType = Kolab::guessFolderTypeFromName( col.name() );

    if ( folderType != Kolab::Mail )
      existingDefaultFolders[ folderType ] = col;
    else if ( guessedType != Kolab::Mail )
      recoveryCandidates[ guessedType ] = col;
  }

  // create/fix folders
  for ( int i = Kolab::Contact; i < Kolab::FolderTypeSize; ++i ) {
    QString iconName;
    if ( i == Kolab::Mail )
    {
      //Nothing
    }
    else if ( i == Kolab::Contact )
    {
      iconName = QString::fromLatin1( "view-pim-contacts" );
    }
    else if ( i == Kolab::Event )
    {
      iconName = QString::fromLatin1( "view-calendar" );
    }
    else if ( i == Kolab::Task )
    {
      iconName = QString::fromLatin1( "view-pim-tasks" );
    }
    else if ( i == Kolab::Journal )
    {
      iconName = QString::fromLatin1( "view-pim-journal" );
    }
    else if ( i == Kolab::Note )
    {
      iconName = QString::fromLatin1( "view-pim-notes" );
    }
    if ( existingDefaultFolders[ i ].isValid() ) {
      continue; // all good
    } else if ( recoveryCandidates[ i ].isValid() ) {
      Collection col = recoveryCandidates[ i ];
      CollectionAnnotationsAttribute* attr = col.attribute<CollectionAnnotationsAttribute>( Entity::AddIfMissing );
      QMap<QByteArray, QByteArray> annotations;
      annotations.insert( KOLAB_FOLDER_TYPE_ANNOTATION, Kolab::folderTypeToString( static_cast<Kolab::FolderType>( i ), true ) );
      attr->setAnnotations( annotations );
      if ( !iconName.isEmpty() ) {
        Akonadi::EntityDisplayAttribute *attribute =  col.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );
        attribute->setIconName( iconName );
      }

      new CollectionModifyJob( col, 0 );
    } else {
      Collection col;
      col.setName( Kolab::nameForFolderType( static_cast<Kolab::FolderType>( i ) ) );
      col.setParentCollection( defaultParent );
      CollectionAnnotationsAttribute* attr = col.attribute<CollectionAnnotationsAttribute>( Entity::AddIfMissing );
      QMap<QByteArray, QByteArray> annotations;
      annotations.insert( KOLAB_FOLDER_TYPE_ANNOTATION, Kolab::folderTypeToString( static_cast<Kolab::FolderType>( i ), true ) );
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
