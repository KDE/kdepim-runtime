/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>
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

#include "kmailmigrator.h"

#include "imapsettings.h"
#include "pop3settings.h"
#include "mboxsettings.h"
#include "maildirsettings.h"
#include "mixedmaildirsettings.h"
#include "dimapcachecollectionmigrator.h"
#include "localfolderscollectionmigrator.h"

#include <Mailtransport/Transport>

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
using Akonadi::AgentManager;
using Akonadi::AgentInstance;
using Akonadi::AgentInstanceCreateJob;

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KStandardDirs>
#include <KLocalizedString>
#include <kwallet.h>
using KWallet::Wallet;

using namespace KMail;

/* Enums for use over DBus. Can't include settings.h for each resource because
   all have same class name. If resource interfaces are changed, these will need
   changing too. */
enum ImapEncryption { Unencrypted = 1, Ssl, Tls };
enum ImapAuthentication { ClearText = 0, Login, Plain, CramMD5, DigestMD5, NTLM,
                          GSSAPI, Anonymous };
enum MboxLocking { Procmail, MuttDotLock, MuttDotLockPrivileged, MboxNone };

static void migratePassword( const QString &idString, const AgentInstance &instance,
                             const QString &newFolder )
{
  QString password;
  Wallet *wallet = Wallet::openWallet( Wallet::NetworkWallet(), 0 );
  if ( wallet && wallet->isOpen() && wallet->hasFolder( "kmail" ) ) {
    wallet->setFolder( "kmail" );
    wallet->readPassword( "account-" + idString, password );
    wallet->setFolder( newFolder );
    wallet->writePassword( instance.identifier() + "rc" , password );
  }
  delete wallet;
}

KMailMigrator::KMailMigrator() :
  KMigratorBase(),
  mConfig( 0 ),
  mConverter( 0 )
{
}

KMailMigrator::~KMailMigrator()
{
  delete mConfig;
}

void KMailMigrator::migrate()
{
  emit message( Info, i18n("Beginning KMail migration...") );
  const QString &kmailCfgFile = KStandardDirs::locateLocal( "config", QString( "kmailrc" ) );
  mConfig = new KConfig( kmailCfgFile );
  mAccounts = mConfig->groupList().filter( QRegExp( "Account \\d+" ) );
  mIt = mAccounts.begin();
  migrateNext();
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
    migrationDone();
    return;
  }

  const KConfigGroup cfgGroup( mConfig, "General" );
  const QString localMaildirDefaultPath = KStandardDirs::locateLocal( "data", QLatin1String( "kmail/mail" ) );
  mLocalMaildirPath = cfgGroup.readPathEntry( "folders", localMaildirDefaultPath );
  const QFileInfo fileInfo( mLocalMaildirPath );
  if ( !fileInfo.exists() || !fileInfo.isDir() ) {
    migrationDone();
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
  deleteLater();
}

void KMailMigrator::migrationFailed( const QString &errorMsg,
                                     const AgentInstance &instance )
{
  const KConfigGroup group( mConfig, mCurrentAccount );
  emit message( Error, i18n( "Migration of '%1' to akonadi resource failed: %2",
                             group.readEntry( "Name " ), errorMsg ) );

  if ( instance.isValid() )
    AgentManager::self()->removeInstance( instance );

  mCurrentAccount.clear();
  migrateNext();
}

void KMailMigrator::migrationCompleted( const AgentInstance &instance  )
{
  const KConfigGroup group( mConfig, mCurrentAccount );
  setMigrationState( group.readEntry( "Id" ), Complete, instance.identifier(),
                     group.readEntry( "Type" ).toLower() );
  emit message( Success, i18n( "Migration of '%1' succeeded.", group.readEntry( "Name" ) ) );
  mCurrentAccount.clear();
  migrateNext();
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
    iface->setSafety( Ssl );
  else if ( config.readEntry( "use-tls" ).toLower() == "true" )
    iface->setSafety( Tls );
  else
    iface->setSafety( Unencrypted );
  const QString &authentication = config.readEntry( "auth" ).toUpper();
  if ( authentication == "LOGIN" )
    iface->setAuthentication( Login );
  else if ( authentication == "PLAIN" )
    iface->setAuthentication( Plain );
  else if ( authentication == "CRAM-MD5" )
    iface->setAuthentication( CramMD5 );
  else if ( authentication == "DIGEST-MD5" )
    iface->setAuthentication( DigestMD5 );
  else if ( authentication == "NTLM" )
    iface->setAuthentication( NTLM );
  else if ( authentication == "GSSAPI" )
    iface->setAuthentication( GSSAPI );
  else if ( authentication == "ANONYMOUS" )
    iface->setAuthentication( Anonymous );
  else
    iface->setAuthentication( ClearText );
  if ( config.readEntry( "subscribed-folders" ).toLower() == "true" )
    iface->setSubscriptionEnabled( true );

  if ( config.readEntry( "check-exclude" ).toLower() == "false" )
    iface->setIntervalCheckTime( config.readEntry( "check-interval", 0 ) );
  else
    iface->setIntervalCheckTime( -1 ); //Exclude

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

  instance.setName( config.readEntry( "Name" ) );
  instance.reconfigure();

  if ( disconnected ) {
#if 0
    DImapCacheCollectionMigrator *collectionMigrator = new DImapCacheCollectionMigrator( instance, this );
    collectionMigrator->setCacheFolder( config.readEntry( "Folder" ) );
    connect( collectionMigrator, SIGNAL( message( int, QString ) ),
             SLOT ( collectionMigratorMessage( int, QString ) ) );
    connect( collectionMigrator, SIGNAL( migrationFinished( Akonadi::AgentInstance, QString ) ),
            SLOT( dimapFoldersMigrationFinished( Akonadi::AgentInstance, QString ) ) );
#else
    migrationCompleted( instance );
#endif
  } else {
    migrationCompleted( instance );
  }
}

void KMailMigrator::pop3AccountCreated( KJob *job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created pop3 resource" ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiPOP3SettingsInterface *iface = new OrgKdeAkonadiPOP3SettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if ( !iface->isValid() ) {
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
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
  iface->setIntervalCheckEnabled( config.readEntry( "check-exclude", false ) );
  iface->setIntervalCheckInterval( config.readEntry( "check-interval", 0 ) );

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

  //TODO port "Folder" to akonadi collection id
  //Info: there is trash item in config which is default and we can't configure it => don't look at it in pop account.
  config.deleteEntry("trash");
  config.deleteEntry("use-default-identity");
  instance.setName( config.readEntry( "Name" ) );
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
  const KConfigGroup config( mConfig, mCurrentAccount );

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

  instance.setName( config.readEntry( "Name" ) );
  instance.reconfigure();
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
  const KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiMaildirSettingsInterface *iface = new OrgKdeAkonadiMaildirSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( i18n("Failed to obtain D-Bus interface for remote configuration."), instance );
    return;
  }

  iface->setPath( config.readEntry( "Location" ) );

  instance.setName( config.readEntry( "Name" ) );
  instance.reconfigure();
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
  instance.reconfigure();

  LocalFoldersCollectionMigrator *collectionMigrator = new LocalFoldersCollectionMigrator( instance, this );
  collectionMigrator->setKMailConfig( mConfig );
  connect( collectionMigrator, SIGNAL( message( int, QString ) ),
           SLOT ( collectionMigratorMessage( int, QString ) ) );
  connect( collectionMigrator, SIGNAL( migrationFinished( Akonadi::AgentInstance, QString ) ),
           SLOT( localFoldersMigrationFinished( Akonadi::AgentInstance, QString ) ) );
}

void KMailMigrator::localFoldersMigrationFinished( const AgentInstance &instance, const QString &error )
{
  if ( error.isEmpty() ) {
    setMigrationState( "LocalFolders", Complete, instance.identifier(), "LocalFolders" );
    emit message( Success, i18n( "Local folders migrated successfully." ) );
    migrationDone();
  } else {
    migrationFailed( error, instance );
  }
}

void KMailMigrator::dimapFoldersMigrationFinished( const AgentInstance &instance, const QString &error )
{
  if ( error.isEmpty() ) {
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

#include "kmailmigrator.moc"
