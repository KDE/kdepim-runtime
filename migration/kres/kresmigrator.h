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

#include <kresources/manager.h>
#include <kresources/resource.h>

#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

template <typename T> class KResMigrator : public QObject
{
  public:
    KResMigrator() : mConfig( 0 )
    {
      KGlobal::ref();
    }

    virtual ~KResMigrator()
    {
      delete mConfig;
      delete mManager;
      KGlobal::deref();
    }

    void migrateType( const QString &type )
    {
      mManager = new KRES::Manager<T>( type );
      mManager->readConfig();
      const QString kresCfgFile = KStandardDirs::locateLocal( "config", QString( "kresources/%1/stdrc" ).arg( type ) );
      mConfig = new KConfig( kresCfgFile );
      mIt = mManager->begin();
      migrateNext();
    }

    void migrateNext()
    {
      while ( mIt != mManager->end() ) {
        KConfigGroup cfg( KGlobal::config(), "Resource " + (*mIt)->identifier() );
        if ( !cfg.readEntry( "Migrated", false ) ) {
          migrateResource( *mIt );
          ++mIt;
          return;
        }
        ++mIt;
      }
      if ( mIt == mManager->end() )
        deleteLater();
    }

    virtual void migrateResource( T *res ) = 0;

    void resourceMigrated( T *res ) const
    {
      KConfigGroup cfg( KGlobal::config(), "Resource " + res->identifier() );
      cfg.writeEntry( "Migrated", true );
      cfg.sync();
    }

    KConfigGroup kresConfig( T* res ) const
    {
      return KConfigGroup( mConfig, "Resource_" + res->identifier() );
    }

  private:
    KConfig *mConfig;
    KRES::Manager<T> *mManager;
    typedef typename KRES::Manager<T>::Iterator ResourceIterator;
    ResourceIterator mIt;
};

#endif
