/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>
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

#include "kmailmigrator.h"

#include "messagetag.h"

// avoid metatype.h from interfering
#include <Nepomuk/Variant>
#define POP3_METATYPE_H

#include "imapsettings.h"
#include "pop3settings.h"
#include "mboxsettings.h"
#include "maildirsettings.h"
#include "mixedmaildirsettings.h"
#include "mixedmaildirstore.h"
#include "emptyresourcecleaner.h"
#include "imapcachecollectionmigrator.h"
#include "imapcachelocalimporter.h"
#include "localfolderscollectionmigrator.h"

#include <KIMAP/LoginJob>
#include <Mailtransport/Transport>

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
using Akonadi::AgentManager;
using Akonadi::AgentInstance;
using Akonadi::AgentInstanceCreateJob;

#include <Nepomuk/Tag>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KStandardDirs>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <kwallet.h>
using KWallet::Wallet;

using namespace KMail;

/* Enums for use over DBus. Can't include settings.h for each resource because
   all have same class name. If resource interfaces are changed, these will need
   changing too. */
enum MboxLocking { Procmail, MuttDotLock, MuttDotLockPrivileged, MboxNone };

static void migratePassword( const QString &idString, const AgentInstance &instance,
                             const QString &newFolder )
{
  QString password;
  Wallet *wallet = Wallet::openWallet( Wallet::NetworkWallet(), 0 );
  if ( wallet && wallet->isOpen() && wallet->hasFolder( "kmail" ) ) {
    wallet->setFolder( "kmail" );
    wallet->readPassword( "account-" + idString, password );
    if ( !wallet->hasFolder( newFolder ) )
      wallet->createFolder( newFolder );
    wallet->setFolder( newFolder );
    wallet->writePassword( instance.identifier() + "rc" , password );
  }
  delete wallet;
}

static MixedMaildirStore *createCacheStore( const QString &basePath )
{
  MixedMaildirStore *store = 0;
  const QFileInfo baseDir( basePath );
  if ( baseDir.exists() && baseDir.isDir() ) {
    store = new MixedMaildirStore;
    store->setPath( baseDir.absoluteFilePath() );
    return store;
  }

  return 0;
}

KMailMigrator::KMailMigrator() :
  KMigratorBase(),
  mDeleteCacheAfterImport( false ),
  mDImapCache( 0 ),
  mImapCache( 0 ),
  mRunningCacheImporterCount( 0 ),
  mLocalFoldersDone( false )
{
  connect( AgentManager::self(), SIGNAL( instanceStatusChanged( Akonadi::AgentInstance ) ),
           this, SLOT( instanceStatusChanged( Akonadi::AgentInstance ) ) );
  connect( AgentManager::self(), SIGNAL( instanceProgressChanged( Akonadi::AgentInstance ) ),
           this, SLOT( instanceProgressChanged( Akonadi::AgentInstance ) ) );
}

KMailMigrator::~KMailMigrator()
{
  delete mDImapCache;
  delete mImapCache;
}

void KMailMigrator::migrate()
{
  emit message( Info, i18n("Beginning KMail migration...") );

  KSharedConfigPtr oldConfig = KSharedConfig::openConfig( QLatin1String( "kmailrc" ) );
  mConfig = KSharedConfig::openConfig( QLatin1String( "kmail2rc" ) );

  const QFileInfo migratorConfigInfo( KStandardDirs::locateLocal( "config", KGlobal::config()->name() ) );

  const QString &newKMailCfgFile = KStandardDirs::locateLocal( "config", QString( "kmail2rc" ) );

  // copy old config into new config, if
  bool copy = false;
  // there is no migrator config (no migration happened yet or full rerun)
  copy = copy || !migratorConfigInfo.exists();
  // new config does not exist yet
  copy = copy || !QFileInfo( newKMailCfgFile ).exists();
  // new config is empty
  copy = copy || mConfig->groupList().isEmpty();
  // new config does not contain any account groups
  copy = copy || mConfig->groupList().filter( QRegExp( "Account \\d+" ) ).isEmpty();

  if ( copy ) {
    kDebug() << "Copying content from kmailrc";
    oldConfig->copyTo( newKMailCfgFile, mConfig.data() );
  } else {
    kDebug() << "Ignoring kmailrc, just using contents from kmail2rc";
  }

  mEmailIdentityConfig = KSharedConfig::openConfig( QLatin1String( "emailidentities" ) );

  deleteOldGroup();
  migrateTags();
  migrateRCFiles();

  mAccounts = mConfig->groupList().filter( QRegExp( "Account \\d+" ) );

  evaluateCacheHandlingOptions();

  mIt = mAccounts.begin();
  migrateNext();
}

void KMailMigrator::deleteOldGroup()
{
  deleteOldGroup( "GroupwareFolderInfo" );
  deleteOldGroup( "Groupware" );
  deleteOldGroup( "IMAP Resource" );
  deleteOldGroup( "Folder_local_root" );
  deleteOldGroup( "Folder_search" );
}

void KMailMigrator::deleteOldGroup( const QString& name ) {
  if ( mConfig->hasGroup( name ) ) {
    KConfigGroup groupName( mConfig, name );
    groupName.deleteGroup();
  }
}

void KMailMigrator::migrateTags()
{
  KConfigGroup tagMigrationConfig( KGlobal::config(), QLatin1String( "MessageTags" ) );
  const QStringList migratedTags = tagMigrationConfig.readEntry( "MigratedTags", QStringList() );

  const QStringList tagGroups = mConfig->groupList().filter( QRegExp( "MessageTag #\\d+" ) );

  QSet<QString> groupNames = tagGroups.toSet();
  groupNames.subtract( migratedTags.toSet() );

  kDebug() << "Tag Groups:" << tagGroups << "MigratedTags:" << migratedTags
           << "Unmigrated Tags:" << groupNames;

  QStringList newlyMigratedTags;
  Q_FOREACH( const QString &groupName, groupNames ) {
    const KConfigGroup group( mConfig, groupName );

    const QString label = group.readEntry( QLatin1String( "Label" ), QString() );
    if ( label.isEmpty() ) {
      continue;
    }
    const QString name = group.readEntry( QLatin1String( "Name" ), QString() );
    if ( name.isEmpty() ) {
      continue;
    }

    const QColor textColor = group.readEntry( QLatin1String( "text-color" ), QColor() );
    const QColor backgroundColor = group.readEntry( QLatin1String( "background-color" ), QColor() );
    const bool hasFont = group.hasKey( QLatin1String( "text-font" ) );
    const QFont textFont = group.readEntry( QLatin1String( "text-font" ), QFont() );

    const bool inToolbar = group.readEntry( QLatin1String( "toolbar" ), false );
    const QString iconName = group.readEntry( QLatin1String( "icon" ), QString::fromLatin1( "mail-tagged" ) );

    const QString shortcut = group.readEntry( QLatin1String( "shortcut" ), QString() );

    Nepomuk::Tag nepomukTag( label );
    nepomukTag.setLabel( name );
    nepomukTag.setSymbols( QStringList( iconName ) );
    nepomukTag.setProperty( Vocabulary::MessageTag::inToolbar(), inToolbar );

    if ( textColor.isValid() ) {
      nepomukTag.setProperty( Vocabulary::MessageTag::textColor(), textColor.name() );
    }

    if ( backgroundColor.isValid() ) {
      nepomukTag.setProperty( Vocabulary::MessageTag::backgroundColor(), backgroundColor.name() );
    }

    if ( hasFont ) {
      nepomukTag.setProperty( Vocabulary::MessageTag::font(), textFont.toString() );
    }

    if ( !shortcut.isEmpty() ) {
      nepomukTag.setProperty( Vocabulary::MessageTag::shortcut(), shortcut );
    }

    kDebug() << "Nepomuk::Tag: label=" << label << "name=" << name
             << "textColor=" << textColor << "backgroundColor=" << backgroundColor
             << "hasFont=" << hasFont << "font=" << textFont
             << "icon=" << iconName << "inToolbar=" << inToolbar
             << "shortcut=" << shortcut;

    newlyMigratedTags << groupName;
  }

  if ( !newlyMigratedTags.isEmpty() ) {
    tagMigrationConfig.writeEntry( "MigratedTags", migratedTags + newlyMigratedTags );
    tagMigrationConfig.sync();
  }
}

void KMailMigrator::migrateRCFiles()
{
  const QDir sourceDir( KStandardDirs::locateLocal( "data", "kmail" ) );
  const QDir targetDir( KStandardDirs::locateLocal( "data", "kmail2" ) );
  KStandardDirs::makeDir( targetDir.absolutePath() );

  const QFileInfoList files = sourceDir.entryInfoList( QStringList() << QLatin1String( "*.rc" ),
                                                       QDir::Files | QDir::Readable );
  Q_FOREACH( const QFileInfo &fileInfo, files ) {
    QFile file( fileInfo.absoluteFilePath() );
    file.copy( QFileInfo( targetDir, fileInfo.fileName() ).absoluteFilePath() );
  }
}

void KMailMigrator::migrateNext()
{
  while ( mIt != mAccounts.end() ) {
    mCurrentAccount = *mIt;
    const KConfigGroup group( mConfig, mCurrentAccount );

    const QString name = group.readEntry( "Name" );
    const QString idString = group.readEntry( "Id" );
    if ( migrationState( idString ) == None ) {
      ++mIt;
      if ( !migrateCurrentAccount() ) {
        emit message( Error, i18n( "No backend for '%1' available.", name ) );
        migrateNext();
      }
      return;
    }

    if ( migrationState( idString ) == Complete )
      emit message( Skip, i18n( "'%1' has been already migrated.", name ) );

    ++mIt;
  }
  if ( mIt == mAccounts.end() )
    migrateLocalFolders();
}

void KMailMigrator::migrateLocalFolders()
{
  if ( migrationState( "LocalFolders" ) == Complete ) {
    emit message( Skip, i18n( "Local folders have already been migrated." ) );
    mLocalFoldersDone = true;
    if ( mRunningCacheImporterCount == 0 ) {
      migrationDone();
    }
    return;
  }

  KConfigGroup cfgGroup( mConfig, "General" );
  cfgGroup.deleteEntry( "QuotaUnit" );
  cfgGroup.deleteEntry( "default-mailbox-format" );
  mConfig->sync();
  const QString localMaildirDefaultPath = KStandardDirs::locateLocal( "data", QLatin1String( "kmail/mail" ) );
  mLocalMaildirPath = cfgGroup.readPathEntry( "folders", localMaildirDefaultPath );
  const QFileInfo fileInfo( mLocalMaildirPath );
  if ( !fileInfo.exists() || !fileInfo.isDir() ) {
    mLocalFoldersDone = true;
    if ( mRunningCacheImporterCount == 0 ) {
      migrationDone();
    }
  } else {
    kDebug() << mLocalMaildirPath;

    emit message( Info, i18nc( "@info:status", "Migrating local folders in '%1'...", mLocalMaildirPath ) );

    createAgentInstance( "akonadi_mixedmaildir_resource", this,
                         SLOT( localMaildirCreated( KJob * ) ) );
  }
}

void KMailMigrator::migrationDone()
{
  emit message( Info, i18n( "Migration successfully completed." ) );

  mConfig->sync();
  kDebug() << "Deleting" << mFailedInstances.count() << "failed resource instances";
  Q_FOREACH( const AgentInstance &instance, mFailedInstances ) {
    if ( instance.isValid() ) {
      AgentManager::self()->removeInstance( instance );
    }
  }
  deleteLater();
}

void KMailMigrator::migrationFailed( const QString &errorMsg,
                                     const AgentInstance &instance )
{
  const KConfigGroup group( mConfig, mCurrentAccount );
  emit message( Error, i18n( "Migration of '%1' to Akonadi resource failed: %2",
                             group.readEntry( "Name" ), errorMsg ) );

  if ( instance.isValid() ) {
    mFailedInstances << instance;
  }

  mCurrentAccount.clear();
  mCurrentInstance = AgentInstance();
  migrateNext();
}

void KMailMigrator::migrationCompleted( const AgentInstance &instance  )
{
  const KConfigGroup group( mConfig, mCurrentAccount );
  setMigrationState( group.readEntry( "Id" ), Complete, instance.identifier(),
                     group.readEntry( "Type" ).toLower() );
  emit message( Success, i18n( "Migration of '%1' succeeded.", group.readEntry( "Name" ) ) );
  mCurrentAccount.clear();
  mCurrentInstance = AgentInstance();
  migrateNext();
}

void KMailMigrator::connectCollectionMigrator( AbstractCollectionMigrator *migrator )
{
  connect( migrator, SIGNAL( message( int, QString ) ),
           SLOT ( collectionMigratorMessage( int, QString ) ) );
  connect( migrator, SIGNAL( status( QString ) ), SIGNAL ( status( QString ) ) );
  connect( migrator, SIGNAL( progress( int ) ), SIGNAL ( progress( int ) ) );
  connect( migrator, SIGNAL( progress( int, int, int ) ), SIGNAL ( progress( int, int, int ) ) );
  connect( migrator, SIGNAL( migrationFinished( Akonadi::AgentInstance, QString ) ),
           this, SLOT( collectionMigratorFinished() ) );
}

void KMailMigrator::evaluateCacheHandlingOptions()
{
  bool needsAction = false;
  Q_FOREACH( const QString &account, mAccounts ) {
    const KConfigGroup accountGroup( mConfig, account );
    if ( migrationState( accountGroup.readEntry( "Id" ) ) != Complete ) {
      const QString type = accountGroup.readEntry( QLatin1String( "Type" ) ).toLower();
      if ( type == QLatin1String( "dimap" ) ) {
        needsAction = true;
        break;
      }
    }
  }

  if ( needsAction ) {
    const QString message =
      i18nc( "@info", "<para>Cached IMAP accounts have a local copy of the server's data.</para>"
                      "<para>These copies can be kept in local folders or be deleted"
                      "after import.</para>"
                      "<note>Mail that did not get imported will always be kept in local folders"
                      "</note>" );

    int result = KMessageBox::questionYesNo( 0, message, i18n( "KMail 2 Migration" ),
                                             KGuiItem( i18nc( "@action", "Delete Copies" ) ),
                                             KGuiItem( i18nc( "@action", "Keep Copies" ) ) );
    mDeleteCacheAfterImport = ( result == KMessageBox::Yes );
  }
}


bool KMailMigrator::migrateCurrentAccount()
{
  if ( mCurrentAccount.isEmpty() )
    return false;
  const KConfigGroup group( mConfig, mCurrentAccount );

  emit message( Info, i18n( "Trying to migrate '%1' to resource...", group.readEntry( "Name" ) ) );

  const QString type = group.readEntry( "Type" ).toLower();
  if ( type == "imap" ) {
    createAgentInstance( "akonadi_imap_resource", this,
                         SLOT( imapAccountCreated( KJob * ) ) );
  }
  else if ( type == "dimap" ) {
    createAgentInstance( "akonadi_imap_resource", this,
                         SLOT( imapDisconnectedAccountCreated( KJob * ) ) );

  }
  else if ( type == "pop" ) {
    createAgentInstance( "akonadi_pop3_resource", this,
                         SLOT( pop3AccountCreated( KJob * ) ) );
  }
  else if ( type == "maildir" ) {
    createAgentInstance( "akonadi_maildir_resource", this,
                         SLOT( maildirAccountCreated( KJob * ) ) );
  }
  else if ( type == "local" ) {
    createAgentInstance( "akonadi_mbox_resource", this,
                         SLOT( mboxAccountCreated( KJob * ) ) );
  }
  else {
    return false;
  }

  return true;
}

void KMailMigrator::imapDisconnectedAccountCreated( KJob * job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created disconnected imap resource" ) );
  migrateImapAccount( job, true );
}

void KMailMigrator::imapAccountCreated( KJob *job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created imap resource" ) );
  migrateImapAccount( job, false );
}


void KMailMigrator::migrateImapAccount( KJob *job, bool disconnected )
{
  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  mCurrentInstance = instance;
  const KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiImapSettingsInterface *iface = new OrgKdeAkonadiImapSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), instance );
    return;
  }

  iface->setImapServer( config.readEntry( "host" ) );
  iface->setImapPort( config.readEntry( "port", 143 ) );
  iface->setUserName( config.readEntry( "login" ) );
  if ( config.readEntry( "use-ssl" ).toLower() == "true" )
    iface->setSafety( "SSL" );
  else if ( config.readEntry( "use-tls" ).toLower() == "true" )
    iface->setSafety( "STARTTLS" );
  else
    iface->setSafety( "NONE" );
  const QString authentication = config.readEntry( "auth" ).toUpper();
  if ( authentication == "LOGIN" )
    iface->setAuthentication(  MailTransport::Transport::EnumAuthenticationType::LOGIN );
  else if ( authentication == "PLAIN" )
    iface->setAuthentication( MailTransport::Transport::EnumAuthenticationType::PLAIN );
  else if ( authentication == "CRAM-MD5" )
    iface->setAuthentication( MailTransport::Transport::EnumAuthenticationType::CRAM_MD5 );
  else if ( authentication == "DIGEST-MD5" )
    iface->setAuthentication( MailTransport::Transport::EnumAuthenticationType::DIGEST_MD5 );
  else if ( authentication == "NTLM" )
    iface->setAuthentication( MailTransport::Transport::EnumAuthenticationType::NTLM );
  else if ( authentication == "GSSAPI" )
    iface->setAuthentication( MailTransport::Transport::EnumAuthenticationType::GSSAPI );
  else if ( authentication == "ANONYMOUS" )
    iface->setAuthentication( MailTransport::Transport::EnumAuthenticationType::ANONYMOUS );
  else {
    iface->setAuthentication( MailTransport::Transport::EnumAuthenticationType::CLEAR );
  }
  if ( config.readEntry( "subscribed-folders" ).toLower() == "true" )
    iface->setSubscriptionEnabled( true );

  // skip interval checking so it doesn't interfere with cache importing
  iface->setIntervalCheckTime( -1 ); //exclude

  iface->setSieveSupport( config.readEntry( "sieve-support", false ) );
  iface->setSieveReuseConfig( config.readEntry( "sieve-reuse-config", true ) );
  iface->setSievePort( config.readEntry( "sieve-port", 2000 ) );
  iface->setSieveAlternateUrl( config.readEntry( "sieve-alternate-url" ) );
  iface->setSieveVacationFilename( config.readEntry( "sieve-vacation-filename", "kmail-vacation.siv" ) );
  iface->setDisconnectedModeEnabled( disconnected );
  iface->setAutomaticExpungeEnabled( config.readEntry("auto-expunge", true ) );
  const bool useDefaultIdentity = config.readEntry( "use-default-identity", true );
  iface->setUseDefaultIdentity( useDefaultIdentity );
  if ( !useDefaultIdentity )
    iface->setAccountIdentity( config.readEntry( "identity-id" ).toInt() );

  migratePassword( config.readEntry( "Id" ), instance, "imap" );

  const QString nameAccount = config.readEntry( "Name" );
  instance.setName( nameAccount );
  emit status( nameAccount );
  instance.reconfigure();

  ImapCacheCollectionMigrator::MigrationOptions options = ImapCacheCollectionMigrator::ImportCachedMessages;
  if ( disconnected ) {
    const KConfigGroup dimapConfig( KGlobal::config(), QLatin1String( "Disconnected IMAP" ) );
    if ( dimapConfig.isValid() ) {
      options = ImapCacheCollectionMigrator::ConfigOnly;

      if ( dimapConfig.readEntry( QLatin1String( "ImportNewMessages" ), false ) ) {
        options |= ImapCacheCollectionMigrator::ImportNewMessages;
      }

      if ( dimapConfig.readEntry( QLatin1String( "ImportCachedMessages" ), false ) ) {
        options |= ImapCacheCollectionMigrator::ImportCachedMessages;
      }

      if ( dimapConfig.readEntry( QLatin1String( "RemoveDeletedMessages" ), false ) ) {
        options |= ImapCacheCollectionMigrator::RemoveDeletedMessages;
      }
    }
  }

  MixedMaildirStore *store = 0;
  if ( disconnected ) {
    if ( mDeleteCacheAfterImport ) {
      options |= ImapCacheCollectionMigrator::DeleteImportedMessages;
    }
    if ( mDImapCache == 0 ){
      mDImapCache = createCacheStore( KStandardDirs::locateLocal( "data", QLatin1String( "kmail/dimap" ) ) );
    }
    store = mDImapCache;
  } else {
    if ( mImapCache == 0 ){
      mImapCache = createCacheStore( KStandardDirs::locateLocal( "data", QLatin1String( "kmail/imap" ) ) );
    }
    store = mImapCache;
  }

  ImapCacheCollectionMigrator *collectionMigrator = new ImapCacheCollectionMigrator( instance, store, this );
  collectionMigrator->setMigrationOptions( options );

  kDebug() << "Starting IMAP collection migration: options="
           << collectionMigrator->migrationOptions();
  QString topLevelFolder = config.readEntry( "Folder" );
  if ( topLevelFolder.isEmpty() ) {
    topLevelFolder = config.readEntry( "Id" );
  }
  collectionMigrator->setTopLevelFolder( topLevelFolder );
  collectionMigrator->setKMailConfig( mConfig );
  collectionMigrator->setEmailIdentityConfig( mEmailIdentityConfig );

  if ( disconnected ) {
    ImapCacheLocalImporter *cacheImporter = new ImapCacheLocalImporter( store, this );
    cacheImporter->setTopLevelFolder( collectionMigrator->topLevelFolder() );
    cacheImporter->setAccountName( nameAccount );

    connect( collectionMigrator, SIGNAL( migrationFinished( Akonadi::AgentInstance, QString ) ),
             cacheImporter, SLOT( startImport() ) );
    connect( cacheImporter, SIGNAL( importFinished( Akonadi::AgentInstance, QString ) ),
              SLOT( imapCacheImportFinished( Akonadi::AgentInstance, QString ) ) );
    ++mRunningCacheImporterCount;
  }

  connect( collectionMigrator, SIGNAL( migrationFinished( Akonadi::AgentInstance, QString ) ),
           SLOT( imapFoldersMigrationFinished( Akonadi::AgentInstance, QString ) ) );

  connectCollectionMigrator( collectionMigrator );
}

void KMailMigrator::pop3AccountCreated( KJob *job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created pop3 resource" ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  mCurrentInstance = instance;
  KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiPOP3SettingsInterface *iface = new OrgKdeAkonadiPOP3SettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if ( !iface->isValid() ) {
    migrationFailed( i18n( "Failed to obtain D-Bus interface for remote configuration." ), instance );
    return;
  }

  iface->setHost( config.readEntry( "host", QString() ) );
  iface->setPort( config.readEntry( "port", 110u ) );
  iface->setLogin( config.readEntry( "login", QString() ) );
  if ( config.readEntry( "use-ssl", true ) )
    iface->setUseSSL( true );
  if ( config.readEntry( "use-tls", true ) )
    iface->setUseTLS( true );
  if ( config.readEntry( "store-passwd", false ) )
    iface->setStorePassword( true );
  if ( config.readEntry( "pipelining", false ) )
    iface->setPipelining( true );
  if ( config.readEntry( "leave-on-server", true ) ) {
    iface->setLeaveOnServer( true );
    iface->setLeaveOnServerDays( config.readEntry( "leave-on-server-days", -1 ) );
    iface->setLeaveOnServerCount( config.readEntry( "leave-on-server-count", -1 ) );
    iface->setLeaveOnServerSize( config.readEntry( "leave-on-server-size", -1 ) );
  }
  if ( config.readEntry( "filter-on-server", false ) ) {
    iface->setFilterOnServer( true );
    iface->setFilterCheckSize( config.readEntry( "filter-os-check-size" ).toUInt() );
  }
  const int checkInterval = config.readEntry( "check-interval", 0 );
  if ( checkInterval == 0 )
    iface->setIntervalCheckEnabled( false );
  else {
    iface->setIntervalCheckEnabled( true );
    iface->setIntervalCheckInterval( checkInterval );
  }

  // Akonadi kmail uses enums for storing auth options
  // so we have to convert from the old string representations
  typedef MailTransport::Transport::EnumAuthenticationType AuthType;
  QString authOpt = config.readEntry( "auth" );
  if( authOpt.contains( "PLAIN", Qt::CaseInsensitive ) )
    iface->setAuthenticationMethod( AuthType::PLAIN );
  else if( authOpt.contains( "CRAM", Qt::CaseInsensitive ) )
    iface->setAuthenticationMethod( AuthType::CRAM_MD5 );
  else if( authOpt.contains( "DIGEST", Qt::CaseInsensitive ) )
    iface->setAuthenticationMethod( AuthType::DIGEST_MD5);
  else if( authOpt.contains( "GSSAPI", Qt::CaseInsensitive ) )
    iface->setAuthenticationMethod( AuthType::GSSAPI );
  else if( authOpt.contains( "NTLM", Qt::CaseInsensitive ) )
    iface->setAuthenticationMethod( AuthType::NTLM);
  else if( authOpt.contains( "APOP", Qt::CaseInsensitive ) )
    iface->setAuthenticationMethod( AuthType::APOP );
  else
    iface->setAuthenticationMethod( AuthType::CLEAR );
  iface->setPrecommand( config.readPathEntry( "precommand", QString() ) );
  migratePassword( config.readEntry( "Id" ), instance, "pop3" );

  // POP3 filter from files in kmail appdata
  const QString popFilterFileName = QString::fromLatin1( "kmail/%1:@%2:%3" )
                                      .arg( config.readEntry( "login", QString() ) )
                                      .arg( config.readEntry( "host", QString() ) )
                                      .arg( config.readEntry( "port", 110u ) );

  const KConfig popFilterConfig( popFilterFileName, KConfig::SimpleConfig, "data" );
  const KConfigGroup popFilterGroup( &popFilterConfig, QString() );
  iface->setDownloadLater( popFilterGroup.readEntry( "downloadLater", QStringList() ) );
  iface->setSeenUidList( popFilterGroup.readEntry( "seenUidList", QStringList() ) );
  iface->setSeenUidTimeList( popFilterGroup.readEntry( "seenUidTimeList", QList<int>() ) );

  //Info: there is trash item in config which is default and we can't configure it => don't look at it in pop account.
  config.deleteEntry("trash");
  config.deleteEntry("use-default-identity");
  const QString nameAccount = config.readEntry( "Name" );
  instance.setName( nameAccount );
  emit status( nameAccount );
  instance.reconfigure();
  config.sync();
  migrationCompleted( instance );
}

void KMailMigrator::mboxAccountCreated( KJob *job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created mbox resource" ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  mCurrentInstance = instance;
  KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiMboxSettingsInterface *iface = new OrgKdeAkonadiMboxSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), instance );
    return;
  }

  iface->setPath( config.readEntry( "Location" ) );
  const QString lockType = config.readEntry( "LockType" ).toLower();
  if ( lockType == "procmail_locktype" ) {
    iface->setLockfileMethod( Procmail );
    iface->setLockfile( config.readEntry( "ProcmailLockFile" ) );
  }
  else if ( lockType == "mutt_dotlock" )
    iface->setLockfileMethod( MuttDotLock );
  else if ( lockType == "mutt_dotlock_privileged" )
    iface->setLockfileMethod( MuttDotLockPrivileged );
  else if ( lockType == "none" )
    iface->setLockfileMethod( MboxNone );

  //Info: there is trash item in config which is default and we can't configure it => don't look at it in pop account.
  config.deleteEntry("trash");
  config.deleteEntry( "identity-id" );
  config.deleteEntry( "use-default-identity" );
  //We can't specify folder in akonadi
  config.deleteEntry( "Folder" );

  //TODO check-interval for the moment mbox doesn't support it

  const QString nameAccount = config.readEntry( "Name" );
  instance.setName( nameAccount );
  emit status( nameAccount );
  instance.reconfigure();
  config.sync();
  migrationCompleted( instance );
}

void KMailMigrator::maildirAccountCreated( KJob *job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created maildir resource" ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  mCurrentInstance = instance;
  KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiMaildirSettingsInterface *iface = new OrgKdeAkonadiMaildirSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), instance );
    return;
  }

  iface->setPath( config.readEntry( "Location" ) );
  //Info: there is trash item in config which is default and we can't configure it => don't look at it in pop account.
  config.deleteEntry( "trash" );
  config.deleteEntry( "identity-id" );
  config.deleteEntry( "use-default-identity" );
  //Now in akonadi we can't specify a folder where we put email, it's a specific top root
  config.deleteEntry( "Folder" );

  //TODO: check-interval for the moment maildir doesn't support check-interval.

  const QString nameAccount = config.readEntry( "Name" );
  instance.setName( nameAccount );
  emit status( nameAccount );
  instance.reconfigure();
  config.sync();
  migrationCompleted( instance );
}

void KMailMigrator::localMaildirCreated( KJob *job )
{
  if ( job->error() ) {
    emit message( Error, i18n( "Failed to resource for local folders: %1", job->errorText() ) );
    deleteLater();
    return;
  }
  emit message( Info, i18n( "Created local maildir resource." ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  mCurrentInstance = instance;

  OrgKdeAkonadiMixedMaildirSettingsInterface *iface = new OrgKdeAkonadiMixedMaildirSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), instance );
    return;
  }

  iface->setPath( mLocalMaildirPath );

  KConfig specialMailCollectionsConfig( QLatin1String( "specialmailcollectionsrc" ) );
  KConfigGroup specialMailCollectionsGroup = specialMailCollectionsConfig.group( QLatin1String( "SpecialCollections" ) );

  QString defaultInstanceName;
  QString defaultResourceId = specialMailCollectionsGroup.readEntry( QLatin1String( "DefaultResourceId" ) );
  if ( defaultResourceId.isEmpty() ) {
    kDebug() << "No resource configured for special mail collections, using the migrated"
             << instance.identifier();
    defaultResourceId = instance.identifier();
  } else {
    const AgentInstance defaultInstance = AgentManager::self()->instance( defaultResourceId );
    if ( !defaultInstance.isValid() ) {
      kDebug() << "Resource" << defaultResourceId
               << " configured for special mail collections does not exist, using the migrated"
               << instance.identifier();
      defaultResourceId = instance.identifier();
    } else {
      defaultInstanceName = defaultInstance.name();
    }
  }

  const QString instanceName = i18n("KMail Folders");

  if ( defaultInstanceName.isEmpty() ) {
    specialMailCollectionsGroup.writeEntry( QLatin1String( "DefaultResourceId" ), defaultResourceId );
    specialMailCollectionsGroup.sync();

    emit message( Info,
                  i18nc( "@info:status resource that will provide folder such as outbox, sent",
                         "Using '%1' for default outbox, sent mail, trash, etc.",
                         instanceName ) );
  } else {
    emit message( Info,
                  i18nc( "@info:status resource that will provide folder such as outbox, sent",
                         "Keeping '%1' for default outbox, sent mail, trash, etc.",
                         defaultInstanceName ) );
  }

  instance.setName( instanceName );
  emit status( instanceName );
  instance.reconfigure();

  LocalFoldersCollectionMigrator *collectionMigrator = new LocalFoldersCollectionMigrator( instance, this );
  collectionMigrator->setKMailConfig( mConfig );
  collectionMigrator->setEmailIdentityConfig( mEmailIdentityConfig );

  connect( collectionMigrator, SIGNAL( migrationFinished( Akonadi::AgentInstance, QString ) ),
           SLOT( localFoldersMigrationFinished( Akonadi::AgentInstance, QString ) ) );
  connectCollectionMigrator( collectionMigrator );
}

void KMailMigrator::localFoldersMigrationFinished( const AgentInstance &instance, const QString &error )
{
  if ( error.isEmpty() ) {
    KConfigGroup config( KGlobal::config(), QLatin1String( "SpecialMailCollections" ) );
    if ( config.readEntry( QLatin1String( "TakeOverIfDefaultIsTotallyEmpty" ), false ) ) {
      KConfig specialMailCollectionsConfig( QLatin1String( "specialmailcollectionsrc" ) );
      KConfigGroup specialMailCollectionsGroup = specialMailCollectionsConfig.group( QLatin1String( "SpecialCollections" ) );

      QString defaultResourceId = specialMailCollectionsGroup.readEntry( QLatin1String( "DefaultResourceId" ) );

      AgentInstance defaultResource = AgentManager::self()->instance( defaultResourceId );
      if ( defaultResource.isValid() && defaultResourceId != instance.identifier() ) {
        kDebug() << "Attempting take over of special mail collections role from"
                 << defaultResourceId;

        EmptyResourceCleaner *cleaner = new EmptyResourceCleaner( defaultResource, this );
        cleaner->setCleaningOptions( EmptyResourceCleaner::CheckOnly );
        connect( cleaner, SIGNAL( cleanupFinished( Akonadi::AgentInstance ) ),
                 SLOT( specialColDefaultResourceCheckFinished( Akonadi::AgentInstance ) ) );
        return;
      }
    }

    mLocalFoldersDone = true;
    mCurrentInstance = AgentInstance();

    setMigrationState( "LocalFolders", Complete, instance.identifier(), "LocalFolders" );
    emit message( Success, i18n( "Local folders migrated successfully." ) );
    if ( mRunningCacheImporterCount == 0 ) {
      migrationDone();
    }
  } else {
    mLocalFoldersDone = true;
    mCurrentInstance = AgentInstance();

    migrationFailed( error, instance );
  }
}

void KMailMigrator::imapFoldersMigrationFinished( const AgentInstance &instance, const QString &error )
{
  kDebug() << "imapMigrationFinished: instance=" << instance.identifier()
           << "error=" << error;
  if ( error.isEmpty() ) {
    OrgKdeAkonadiImapSettingsInterface *iface = new OrgKdeAkonadiImapSettingsInterface(
      "org.freedesktop.Akonadi.Resource." + instance.identifier(),
      "/Settings", QDBusConnection::sessionBus(), this );

    if (!iface->isValid() ) {
      migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), instance );
      return;
    }

    // enable interval checking in case this had been configured
    const KConfigGroup config( mConfig, mCurrentAccount );
    const int checkInterval = config.readEntry( "check-interval", 0 );
    if ( checkInterval != 0 ) {
      iface->setIntervalCheckTime( checkInterval );
      instance.reconfigure();
    }

    migrationCompleted( instance );
  } else {
    migrationFailed( error, instance );
  }
}

void KMailMigrator::collectionMigratorMessage( int type, const QString &msg )
{
  switch ( type ) {
    case Success:
      emit message( Success, msg );
      break;

    case Skip:
      emit message( Skip, msg );
      break;

    case Warning:
      emit message( Warning, msg );
      break;

    case Info:
      emit message( Info, msg );
      break;

    case Error:
      emit message( Error, msg );
      break;

    default:
      kError() << "Unknown message type" << type << "msg=" << msg;
      break;
  }
}

void KMailMigrator::collectionMigratorFinished()
{
  emit status( QString() );
}

void KMailMigrator::instanceStatusChanged( const AgentInstance &instance )
{
  if ( instance.identifier() != mCurrentInstance.identifier() ) {
    return;
  }

  if ( instance.status() == AgentInstance::Idle ) {
    emit status( QString() );
  } else {
    emit status( instance.statusMessage() );
  }
}

void KMailMigrator::instanceProgressChanged( const AgentInstance &instance )
{
  if ( instance.identifier() != mCurrentInstance.identifier() ) {
    return;
  }

  emit progress( 0, 100, instance.progress() );
}

void KMailMigrator::imapCacheImportFinished( const Akonadi::AgentInstance &instance, const QString &error )
{
  kDebug() << "CacheImport into" << instance.identifier()
           << "finished (error=" << error << "), "
           << ( mRunningCacheImporterCount - 1 ) << "still running";

  if ( !error.isEmpty() ) {
    --mRunningCacheImporterCount;

    emit message( Error, error );
  } else {
    if ( mDeleteCacheAfterImport ) {
      EmptyResourceCleaner *cleaner = new EmptyResourceCleaner( instance, this );
      cleaner->setCleaningOptions( EmptyResourceCleaner::DeleteEmptyCollections |
                                   EmptyResourceCleaner::DeleteEmptyResource );
     connect( cleaner, SIGNAL( cleanupFinished( Akonadi::AgentInstance ) ),
              SLOT( imapCacheCleanupFinished( Akonadi::AgentInstance ) ) );
    } else {
      --mRunningCacheImporterCount;
    }
  }

  if ( mRunningCacheImporterCount == 0 && mLocalFoldersDone ) {
    migrationDone();
  }
}

void KMailMigrator::imapCacheCleanupFinished( const Akonadi::AgentInstance &instance )
{
  --mRunningCacheImporterCount;

  kDebug() << "CacheCleanup of" << instance.identifier() << "finished, "
           << mRunningCacheImporterCount << "still running";

  if ( mRunningCacheImporterCount == 0 && mLocalFoldersDone ) {
    migrationDone();
  }
}

void KMailMigrator::specialColDefaultResourceCheckFinished( const AgentInstance &instance )
{
  KConfig specialMailCollectionsConfig( QLatin1String( "specialmailcollectionsrc" ) );
  KConfigGroup specialMailCollectionsGroup = specialMailCollectionsConfig.group( QLatin1String( "SpecialCollections" ) );

  const EmptyResourceCleaner *cleaner = qobject_cast<const EmptyResourceCleaner*>( QObject::sender() );

  const QString localFoldersIdentifier = mCurrentInstance.identifier();
  if ( cleaner->isResourceDeletable() ) {
    specialMailCollectionsGroup.writeEntry( QLatin1String( "DefaultResourceId" ), localFoldersIdentifier );
    specialMailCollectionsGroup.sync();
    AgentManager::self()->removeInstance( instance );

    kDebug() << "Former special mail collection resource" << instance.identifier()
             << "successfully delete, now using" << specialMailCollectionsGroup.readEntry( QLatin1String( "DefaultResourceId" ) );
  } else {
    kDebug() << "Former special mail collection resource" << instance.identifier()
             << "still valid";
  }

  mLocalFoldersDone = true;
  mCurrentInstance = AgentInstance();

  setMigrationState( "LocalFolders", Complete, localFoldersIdentifier, "LocalFolders" );
  emit message( Success, i18n( "Local folders migrated successfully." ) );
  if ( mRunningCacheImporterCount == 0 ) {
    migrationDone();
  }
}

#include "kmailmigrator.moc"
