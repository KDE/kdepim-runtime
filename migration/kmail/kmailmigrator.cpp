/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>

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
//#include "pop3settings.h"
#include "mboxsettings.h"
#include "maildirsettings.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
using Akonadi::AgentManager;
using Akonadi::AgentInstance;
using Akonadi::AgentInstanceCreateJob;
#include <resources/mbox/settings.h>

#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KLocalizedString>
#include <kwallet.h>
using KWallet::Wallet;

using namespace KMail;

enum ImapEncryption { Unencrypted = 1, Ssl, Tls };
enum ImapAuthentication { ClearText = 1, Login, Plain, CramMD5, DigestMD5, NTLM,
                          GSSAPI, Anonymous };

static QString retrievePassword( const QString &idString )
{
  QString password;
  Wallet *wallet = Wallet::openWallet( Wallet::NetworkWallet(), 0 );
  if ( wallet && wallet->isOpen() && wallet->hasFolder( "kmail" ) ) {
    wallet->setFolder( "kmail" );
    wallet->readPassword( "account-" + idString, password );
  } else
    kWarning() << "No password retrieved.";
  delete wallet;
  return password;
}

KMailMigrator::KMailMigrator( const QStringList &typesToMigrate ) :
  KMigratorBase(),
  mTypes( typesToMigrate ),
  mConfig( 0 )
{
}

KMailMigrator::~KMailMigrator()
{
  delete mConfig;
}

void KMailMigrator::migrate()
{
  emit message( Info, "Beginning KMail migration" );
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
    if ( !mTypes.contains( group.readEntry( "Type" ).toLower() ) ) {
      ++mIt;
      continue;
    }

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

  mCurrentAccount = QString();
  migrateNext();
}

void KMailMigrator::migrationCompleted( const AgentInstance &instance  )
{
  const KConfigGroup group( mConfig, mCurrentAccount );
  setMigrationState( group.readEntry( "Id" ), Complete, instance.identifier(),
                     group.readEntry( "Type" ).toLower() );
  emit message( Success, i18n( "Migration of '%1' succeeded.", group.readEntry( "Name" ) ) );
  mCurrentAccount = QString();
  //mManager->remove( mCurrentAccount );
  migrateNext();
}

bool KMailMigrator::migrateCurrentAccount()
{
  if ( mCurrentAccount.isEmpty() )
    return false;
  const KConfigGroup group( mConfig, mCurrentAccount );

  emit message( Info, i18n( "Trying to migrate '%1' to resource...", group.readEntry( "Name" ) ) );

  const QString type = group.readEntry( "Type" ).toLower();
  if ( type == "imap" || type == "dimap" ) {
    createAgentInstance( "akonadi_imap_resource", this,
                         SLOT( imapAccountCreated( KJob * ) ) );
  }
  else if ( type == "pop" ) {
    /* createAgentInstance( "akonadi_pop3_resource", this,
                         SLOT( pop3AccountCreated( KJob * ) ) ); */
    return false;
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

void KMailMigrator::imapAccountCreated( KJob *job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created imap resource" ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  const KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiImapSettingsInterface *iface = new OrgKdeAkonadiImapSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
    return;
  }

  iface->setImapServer( config.readEntry( "host" ) + ':' + config.readEntry( "port" ) );
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

  iface->setPassword( retrievePassword( config.readEntry( "Id" ) ) );

  //instance.reconfigure();
  migrationCompleted( instance );
}

/*
void KMailMigrator::pop3AccountCreated( KJob *job )
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  emit message( Info, i18n( "Created pop3 resource" ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();
  const KConfigGroup config( mConfig, mCurrentAccount );

  OrgKdeAkonadiPop3SettingsInterface *iface = new OrgKdeAkonadiPop3SettingsInterface(
    "org.freedesktop.Akonadi.Resource." + instance.identifier(),
    "/Settings", QDBusConnection::sessionBus(), this );

  if (!iface->isValid() ) {
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
    return;
  }

  iface->setHost( config.readEntry( "host" ) );
  iface->setPort( config.readEntry( "port" ).toUInt() );
  iface->setLogin( config.readEntry( "login" ) );
  if ( config.readEntry( "use-ssl" ).toLower() == "true" )
    iface->setUseSSL( true );
  if ( config.readEntry( "use-tls" ).toLower() == "true" )
    iface->setUseTLS( true );
  if ( config.readEntry( "store-passwd" ).toLower() == "true" )
    iface->setStorePassword( true );
  if ( config.readEntry( "pipelining" ).toLower() == "true" )
    iface->setPipelining( true );
  if ( config.readEntry( "leave-on-server" ).toLower() == "true" ) {
    iface->setLeaveOnServer( true );
    iface->setLeaveOnServerDays( config.readEntry( "leave-on-server-days" ).toInt() );
    iface->setLeaveOnServerCount( config.readEntry( "leave-on-server-count" ).toInt() );
    iface->setLeaveOnServerSize( config.readEntry( "leave-on-server-size" ).toInt() );
  }
  if ( config.readEntry( "filter-on-server" ).toLower() == "true" ) {
    iface->setFilterOnServer( true );
    iface->setFilterCheckSize( config.readEntry( "filter-on-server" ).toUInt() );
  }
  iface->setAuthenticationMethod( config.readEntry( "auth" ));

  iface->setPassword( retrievePassword( config.readEntry( "Id" ) ) );

  //instance.reconfigure();
  migrationCompleted( instance );
}
*/

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
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
    return;
  }

  iface->setPath( config.readEntry( "Location" ) );
  const QString lockType = config.readEntry( "LockType" ).toLower();
  if ( lockType == "procmail_locktype" ) {
    iface->setLockfileMethod( Settings::procmail );
    iface->setLockfile( config.readEntry( "ProcmailLockFile" ) );
  }
  else if ( lockType == "mutt_dotlock" )
    iface->setLockfileMethod( Settings::mutt_dotlock );
  else if ( lockType == "mutt_dotlock_privileged" )
    iface->setLockfileMethod( Settings::mutt_dotlock_privileged );
  else if ( lockType == "none" )
    iface->setLockfileMethod( Settings::none );

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
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
    return;
  }

  iface->setPath( config.readEntry( "Location" ) );

  instance.reconfigure();
  migrationCompleted( instance );
}

#include "kmailmigrator.moc"
