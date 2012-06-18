/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#ifndef KMAILMIGRATOR_H
#define KMAILMIGRATOR_H

#include "kmigratorbase.h"

#include <KSharedConfig>

#include <QStringList>
#include <QHash>

class AbstractCollectionMigrator;
class ImapCacheAdapter;
class KJob;
class MixedMaildirStore;
class OrgKdeAkonadiImapSettingsInterface;
class OrgKdeAkonadiPOP3SettingsInterface;

namespace KWallet
{
  class Wallet;
}

namespace KMail
{

/**
 * KMail to Akonadi account migrator
 */
class KMailMigrator : public KMigratorBase
{
  Q_OBJECT

  public:
    KMailMigrator();
    virtual ~KMailMigrator();

    void migrate();

    void migrateTags();
    void migrateRCFiles();
    void migrateNotifyFile();
    void migrateNext();
    void migrateLocalFolders();
    void migrationDone();
    void deleteOldGroup();
    void migrateConfigurationDialogRestriction();

  Q_SIGNALS:
    void status( const QString &msg );
    void progress( int value );
    void progress( int min, int max, int value );

  private Q_SLOTS:
    void imapAccountCreated( KJob *job );
    void imapDisconnectedAccountCreated( KJob *job );
    void pop3AccountCreated( KJob *job );
    void mboxAccountCreated( KJob *job );
    void maildirAccountCreated( KJob *job );
    void localMaildirCreated( KJob *job );

    void localFoldersMigrationFinished( const Akonadi::AgentInstance &instance, const QString &error );
    void imapFoldersMigrationFinished( const Akonadi::AgentInstance &instance, const QString &error );

    void collectionMigratorMessage( int type, const QString &msg );
    void collectionMigratorFinished();
    void collectionMigratorEmittedNotification();

    void instanceStatusChanged( const Akonadi::AgentInstance &instance );
    void instanceProgressChanged( const Akonadi::AgentInstance &instance );

#if 0
    void imapCacheImportFinished( const Akonadi::AgentInstance &instance, const QString &error );
    void imapCacheCleanupFinished( const Akonadi::AgentInstance &instance );
#endif
    void imapCacheAdaptionFinished( int messageType, const QString &message );

    void specialColDefaultResourceCheckFinished( const Akonadi::AgentInstance &instance );

    void autoSaveCopyResult( KJob* );

  private:
    void deleteOldGroup( const QString& );
    void migrateImapAccount( KJob *job, bool disconnected );
    bool migrateCurrentAccount();
    void migrationFailed( const QString &errorMsg, const Akonadi::AgentInstance &instance
                          = Akonadi::AgentInstance() )
    {
      migrationFailed( errorMsg, true, instance );
    }

    void migrationFailed( const QString &errorMsg, bool doMigrateNext,
                          const Akonadi::AgentInstance &instance = Akonadi::AgentInstance() );
    void migrationCompleted( const Akonadi::AgentInstance &instance, bool doMigrateNext = true );

    void connectCollectionMigrator( AbstractCollectionMigrator *migrator );

    void migrateInstanceTrashFolder();

    void migratePassword( const QString &idString, const Akonadi::AgentInstance &instance,
                          const QString &newFolder, const QString &passwordFromFilePassword );

    OrgKdeAkonadiImapSettingsInterface* createImapSettingsInterface( const Akonadi::AgentInstance& instance );
    OrgKdeAkonadiPOP3SettingsInterface* createPop3SettingsInterface( const Akonadi::AgentInstance& instance );

    //code from kmaccount
    QString importPassword( const QString &aStr );
    void cleanupConfigFile();

  private:
    KWallet::Wallet *mWallet;
    KSharedConfigPtr mConfig;
    KSharedConfigPtr mEmailIdentityConfig;
    KSharedConfigPtr mKcmKmailSummaryConfig;
    KSharedConfigPtr mTemplatesConfig;
    QString mCurrentAccount;
    QStringList mAccounts;
    QString mLocalMaildirPath;
    typedef QStringList::iterator AccountIterator;
    AccountIterator mIt;
    Akonadi::AgentInstance mCurrentInstance;
    MixedMaildirStore *mDImapCache;
    MixedMaildirStore *mImapCache;

    struct AccountConfig {
      AccountConfig() : imapAccount( false ), disconnectedImap( false ) { }
      Akonadi::AgentInstance instance;
      bool imapAccount;
      bool disconnectedImap;
    };
    QHash<QString, AccountConfig> mAccountInstance;
    QList<Akonadi::AgentInstance> mFailedInstances;

    bool mForwardResourceNotifications;

    ImapCacheAdapter *mImapCacheAdapter;
};

} // namespace KMail

#endif
