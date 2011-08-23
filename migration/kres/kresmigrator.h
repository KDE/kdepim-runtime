/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef KRESMIGRATOR_H
#define KRESMIGRATOR_H

#include "kresmigratorbase.h"

#include <akonadi/agentmanager.h>
#include <kresources/manager.h>
#include <kresources/resource.h>
#include <QFile>

#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

using namespace Akonadi;

template <typename T> class KResMigrator : public KResMigratorBase
{
  public:
    KResMigrator( const QString &type, const QString &bridgeType ) :
      KResMigratorBase( type, bridgeType ),
      mConfig( 0 ),
      mManager( 0 ),
      mBridgeManager( 0 ),
      mClientBridgeFound( false )
    {
    }

    virtual ~KResMigrator()
    {
      delete mBridgeManager;
      mBridgeManager = 0;
      delete mManager;
      mManager = 0;
      delete mConfig;
      mConfig = 0;
    }

    void migrate()
    {
      const QString kresCfgFile = KStandardDirs::locateLocal( "config", QString( "kresources/%1/stdrc" ).arg( mType ) );
      mConfig = new KConfig( kresCfgFile );
      const KConfigGroup generalGroup( mConfig, QLatin1String( "General" ) );
      mUnknownTypeResources = QSet<QString>::fromList( generalGroup.readEntry( QLatin1String( "ResourceKeys" ), QStringList() ) );
      
      mManager = new KRES::Manager<T>( mType );
      mManager->readConfig();
      {
        ResourceIterator it = mManager->begin();
        while( it != mManager->end() ) {
          mResourcesToMigrate.append( *it );
          ++it;
        }
        mIt = mResourcesToMigrate.constBegin();
      }
      migrateNext();
    }

    void migrateNext()
    {
      while ( mIt != mResourcesToMigrate.constEnd() ) {
        mUnknownTypeResources.remove( (*mIt)->identifier() );
        if ( (*mIt)->type() == "akonadi" ) {
          mClientBridgeIdentifier = (*mIt)->identifier();
          mClientBridgeFound = true;
          emit message( Skip, i18n( "Client-side bridge already set up." ) );
          ++mIt;
          continue;
        }
        KConfigGroup cfg( KGlobal::config(), "Resource " + (*mIt)->identifier() );
        kDebug() << "migrateNext:" << (*mIt)->identifier();
        if ( migrationState( (*mIt)->identifier() ) == None ) {
          emit message( Info, i18n( "Trying to migrate '%1'...", (*mIt)->resourceName() ) );
          mPendingBridgedResources.removeAll( (*mIt)->identifier() );
          T* res = *mIt;
          mCurrentKResource = res;
          ++mIt;

          if ( res->type() == "imap" ) {
            createKolabResource( res->identifier(), res->resourceName() );
          } else {
            bool nativeAvailable = mBridgeOnly ? false : migrateResource( res );
            if ( !nativeAvailable ) {
              emit message( Info, i18n( "No native backend for '%1' available.", res->resourceName() ) );
              migrateToBridge( res, mBridgeType );
            }
          }
          return;
        }
        if ( migrationState( (*mIt)->identifier() ) == Bridged &&
             !mPendingBridgedResources.contains( (*mIt)->identifier() ) )
          mPendingBridgedResources << (*mIt)->identifier();
        if ( migrationState( (*mIt)->identifier() ) == Complete )
          emit message( Skip, i18n( "'%1' has already been migrated.", (*mIt)->resourceName() ) );
        ++mIt;
      }
      if ( !mBridgeType.isEmpty() ) {
        if ( mIt == mResourcesToMigrate.constEnd() ) {
          migrateBridged();
          if ( mPendingBridgedResources.isEmpty() ) {
            migrateUnknown();
            if ( mUnknownTypeResources.isEmpty() ) {
              deleteLater();
            }
          }
        }
      } else {
        migrateUnknown();
        if ( mUnknownTypeResources.isEmpty() ) {
          deleteLater();
        }
      }
    }

    void migrateBridged()
    {
      delete mBridgeManager;
      mBridgeManager = 0;

      if ( mPendingBridgedResources.isEmpty() ) {
        setupClientBridge();
        return;
      }
      const QString resId = mPendingBridgedResources.takeFirst();
      KConfigGroup resMigrationCfg( KGlobal::config(), "Resource " + resId );
      const QString akoResId = resMigrationCfg.readEntry( "ResourceIdentifier", "" );
      if ( akoResId.isEmpty() ) {
        mUnknownTypeResources.remove( resId );
        emit message( Error, i18n("No Akonadi agent identifier specified for previously bridged resource '%1'", resId ));
        migrateNext();
        return;
      }

      const QString bridgedCfgFile = KStandardDirs::locateLocal( "config", QString( "%1rc" ).arg( akoResId ) );
      kDebug() << bridgedCfgFile;
      if ( !QFile::exists( bridgedCfgFile ) ) {
          emit message( Info, i18n( "Bridged resource %1 doesn't exist anymore, let's try a direct migration", akoResId ) );
          setMigrationState( resId /*old id*/, None,
                             QString() /*akonadi id*/, mType );

          ResourceIterator it = mManager->begin();
          while( it != mManager->end() ) {
              if ((*it)->identifier() == resId) {
                  mResourcesToMigrate.append( *it );
                  break;
              }
              ++it;
          }
          migrateNext();
          return;
      }
      mUnknownTypeResources.remove( resId );
      mConfig = new KConfig( bridgedCfgFile );

      mBridgeManager = new KRES::Manager<T>( mType );
      mBridgeManager->readConfig( mConfig );
      if ( !mBridgeManager->standardResource() ) {
        // the plugin for this type might no longer be available
        const KConfigGroup kresCfgGroup( mConfig, QString::fromLatin1( "Resource_%1").arg( resId ) );
        const QString resourceType = kresCfgGroup.readEntry( QLatin1String( "ResourceType" ), QString() );
        if ( !resourceType.isEmpty() ) {
          if ( resourceType == QLatin1String( "imap" ) ) {
            createKolabResource( resId, kresCfgGroup.readEntry( QLatin1String( "ResourceName" ), QString() ) );
            return;
          }

          if ( migrateUnknownResource( resId, resourceType, kresCfgGroup ) ) {
            migrateNext();
            return;
          }
        }
        emit message( Error, i18n("Bridged resource '%1' has no standard resource.", resId) );
        migrateNext();
        return;
      }

      T *res = mBridgeManager->standardResource();
      emit message( Info, i18n( "Trying to migrate '%1' from compatibility bridge to native backend...", res->resourceName() ) );
      mCurrentKResource = res;
      bool nativeAvailable = migrateResource( res );
      if ( !nativeAvailable ) {
        emit message( Skip, i18n( "No native backend available, keeping compatibility bridge for '%1'.", res->resourceName() ) );
        migrateNext();
      }
    }

    virtual bool migrateResource( T *res ) = 0;

    virtual bool migrateUnknownResource( const QString &kresId, const QString &kresType, const KConfigGroup &kresConfig )
    {
      Q_UNUSED( kresId );
      Q_UNUSED( kresType );
      Q_UNUSED( kresConfig );

      migrateNext();
      return false;
    }

    KConfigGroup kresConfig( KRES::Resource* res ) const
    {
      return KConfigGroup( mConfig, "Resource_" + res->identifier() );
    }

    T* currentResource()
    {
      T* res = dynamic_cast<T*>( mCurrentKResource );
      Q_ASSERT( res );
      return res;
    }

    void migrationCompleted( const Akonadi::AgentInstance &instance )
    {
      KResMigratorBase::migrationCompleted( instance, mCurrentKResource->identifier(), mCurrentKResource->resourceName() );
      migrationCompletedHelper( instance );
    }

    void migratedToBridge(const Akonadi::AgentInstance & instance)
    {
      mBridgingInProgress = false;
      setMigrationState( mCurrentKResource->identifier(), Bridged, instance.identifier(), mType );
      emit message( Success, i18n( "Migration of '%1' to compatibility bridge succeeded.", mCurrentKResource->resourceName() ) );
      migrationCompletedHelper( instance );
    }

  private:

    // This is called when a native migration or a migration to a bridged resource is
    // successfully completed.
    void migrationCompletedHelper( const Akonadi::AgentInstance & instance )
    {
      if ( mManager->standardResource() == mCurrentKResource ) {
        mAgentForOldDefaultResource = instance.identifier();
      }
      mManager->setActive( mCurrentKResource, false );
      mCurrentKResource = 0;
      migrateNext();
    }

    void setupClientBridge()
    {
      if ( !mOmitClientBridge ) {
        if ( !mClientBridgeFound ) {
          emit message( Info, i18n( "Setting up client-side bridge..." ) );
          T* clientBridge = mManager->createResource( "akonadi" );
          if ( clientBridge ) {
            mClientBridgeIdentifier = clientBridge->identifier();
            clientBridge->setResourceName( i18n("Akonadi Compatibility Resource") );
            mManager->add( clientBridge );
            mManager->setStandardResource( clientBridge );
            emit message( Info, i18n( "Client-side bridge set up successfully." ) );
          } else {
            emit message( Error, i18n( "Could not create client-side bridge, check if Akonadi KResource bridge is installed." ) );
          }
        }

        mManager->writeConfig();
        const QString keyName( "DefaultAkonadiResourceIdentifier" );
        KConfigGroup clientBridgeConfig( mConfig, "Resource_" + mClientBridgeIdentifier );
        if ( !clientBridgeConfig.hasKey( keyName ) &&
            !mAgentForOldDefaultResource.isEmpty() ) {
          clientBridgeConfig.writeEntry( keyName, mAgentForOldDefaultResource );
        }
      }
    }
    
    void migrateUnknown()
    {
      if ( mUnknownTypeResources.isEmpty() ) {
        return;
      }
      
      QSet<QString>::iterator setIt = mUnknownTypeResources.begin();
      const QString kresId = *setIt;
      mUnknownTypeResources.erase( setIt );
      
      const KConfigGroup kresCfgGroup( mConfig, QString::fromLatin1( "Resource_%1").arg( kresId ) );
      const QString resourceType = kresCfgGroup.readEntry( QLatin1String( "ResourceType" ), QString() );
      const QString resourceName = kresCfgGroup.readEntry( QLatin1String( "ResourceName" ), QString() );

      kDebug() << "migrating Unknown" << resourceName << ", type=" << resourceType << ",id=" << kresId;
      
      if ( resourceType == QLatin1String( "imap" ) ) {
        createKolabResource( kresId, resourceName );
      } else {
        migrateUnknownResource( kresId, resourceType, kresCfgGroup );
      } 
    }

  private:
    KConfig *mConfig;
    KRES::Manager<T> *mManager, *mBridgeManager;
    typedef typename KRES::Manager<T>::Iterator ResourceIterator;
    typename QList<T*>::const_iterator mIt;
    QList<T*> mResourcesToMigrate;
    bool mClientBridgeFound;
    QString mClientBridgeIdentifier;
    QString mAgentForOldDefaultResource;
    QSet<QString> mUnknownTypeResources;
};

#endif
