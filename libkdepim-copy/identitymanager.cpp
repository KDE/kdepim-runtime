/*  -*- mode: C++; c-file-style: "gnu" -*-
    identitymanager.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

// config keys:
static const char configKeyDefaultIdentity[] = "Default Identity";

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "identitymanager.h"

#include "identity.h" // for IdentityList::{export,import}Data
#include "email.h" // for static helper functions

#include <kemailsettings.h> // for IdentityEntry::fromControlCenter()
#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kuser.h>

#include <qregexp.h>

#include <assert.h>

using namespace KPIM;

IdentityManager::IdentityManager( bool readonly, QObject * parent, const char * name )
  : ConfigManager( parent, name ), DCOPObject( "KPIM::IdentityManager" )
{
  mConfig = new KConfig( "emailidentities", readonly );
  mReadOnly = readonly;
  readConfig();
  mShadowIdentities = mIdentities;
  // we need at least a default identity:
  if ( mIdentities.isEmpty() ) {
    kdDebug( 5006 ) << "IdentityManager: No identity found. Creating default." << endl;
    createDefaultIdentity();
    commit();
  }
}

IdentityManager::~IdentityManager()
{
  kdWarning( hasPendingChanges(), 5006 )
    << "IdentityManager: There were uncommitted changes!" << endl;
  delete mConfig;
}

void IdentityManager::commit()
{
  // early out:
  if ( !hasPendingChanges() || mReadOnly ) return;

  QValueList<uint> seenUOIDs;
  for ( QValueList<Identity>::ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it )
    seenUOIDs << (*it).uoid();

  // find added and changed identities:
  for ( QValueList<Identity>::ConstIterator it = mShadowIdentities.begin() ;
	it != mShadowIdentities.end() ; ++it ) {
    QValueList<uint>::Iterator uoid = seenUOIDs.find( (*it).uoid() );
    if ( uoid != seenUOIDs.end() ) {
      const Identity & orig = identityForUoid( *uoid );
      if ( *it != orig ) {
	// changed identity
	kdDebug( 5006 ) << "emitting changed() for identity " << *uoid << endl;
	emit changed( *it );
	emit changed( *uoid );
      }
      seenUOIDs.remove( uoid );
    } else {
      // new identity
      kdDebug( 5006 ) << "emitting added() for identity " << (*it).uoid() << endl;
      emit added( *it );
    }
  }

  // what's left are deleted identities:
  for ( QValueList<uint>::ConstIterator it = seenUOIDs.begin() ;
	it != seenUOIDs.end() ; ++it ) {
    kdDebug( 5006 ) << "emitting deleted() for identity " << (*it) << endl;
    emit deleted( *it );
  }

  mIdentities = mShadowIdentities;
  writeConfig();
  emit ConfigManager::changed();
}

void IdentityManager::rollback()
{
  mShadowIdentities = mIdentities;
}

bool IdentityManager::hasPendingChanges() const
{
  return mIdentities != mShadowIdentities;
}

QStringList IdentityManager::identities() const
{
  QStringList result;
  for ( ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it )
    result << (*it).identityName();
  return result;
}

QStringList IdentityManager::shadowIdentities() const
{
  QStringList result;
  for ( ConstIterator it = mShadowIdentities.begin() ;
	it != mShadowIdentities.end() ; ++it )
    result << (*it).identityName();
  return result;
}

void IdentityManager::sort() {
  qHeapSort( mShadowIdentities );
}

void IdentityManager::writeConfig() const {
  QStringList identities = groupList();
  for ( QStringList::Iterator group = identities.begin() ;
	group != identities.end() ; ++group )
    mConfig->deleteGroup( *group );
  int i = 0;
  for ( ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it, ++i ) {
    KConfigGroup cg( mConfig, QString::fromLatin1("Identity #%1").arg(i) );
    (*it).writeConfig( &cg );
    if ( (*it).isDefault() ) {
      // remember which one is default:
      KConfigGroup general( mConfig, "General" );
      general.writeEntry( configKeyDefaultIdentity, (*it).uoid() );
    }
  }
  mConfig->sync();
}

void IdentityManager::readConfig() {
  mIdentities.clear();

  QStringList identities = groupList();
  if ( identities.isEmpty() ) return; // nothing to be done...

  KConfigGroup general( mConfig, "General" );
  uint defaultIdentity = general.readUnsignedNumEntry( configKeyDefaultIdentity );
  bool haveDefault = false;

  for ( QStringList::Iterator group = identities.begin() ;
	group != identities.end() ; ++group ) {
    KConfigGroup config( mConfig, *group );
    mIdentities << Identity();
    mIdentities.last().readConfig( &config );
    if ( !haveDefault && mIdentities.last().uoid() == defaultIdentity ) {
      haveDefault = true;
      mIdentities.last().setIsDefault( true );
    }
  }
  if ( !haveDefault ) {
    kdWarning( 5006 ) << "IdentityManager: There was no default identity. Marking first one as default." << endl;
    mIdentities.first().setIsDefault( true );
  }
  qHeapSort( mIdentities );
}

QStringList IdentityManager::groupList() const {
  return mConfig->groupList().grep( QRegExp("^Identity #\\d+$") );
}

IdentityManager::ConstIterator IdentityManager::begin() const {
  return mIdentities.begin();
}

IdentityManager::ConstIterator IdentityManager::end() const {
  return mIdentities.end();
}

IdentityManager::Iterator IdentityManager::begin() {
  return mShadowIdentities.begin();
}

IdentityManager::Iterator IdentityManager::end() {
  return mShadowIdentities.end();
}

const Identity & IdentityManager::identityForName( const QString & name ) const
{
  kdWarning( 5006 )
    << "deprecated method IdentityManager::identityForName() called!" << endl;
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) return (*it);
  return Identity::null;
}

const Identity & IdentityManager::identityForUoid( uint uoid ) const {
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( (*it).uoid() == uoid ) return (*it);
  return Identity::null;
}

const Identity & IdentityManager::identityForNameOrDefault( const QString & name ) const
{
  const Identity & ident = identityForName( name );
  if ( ident.isNull() )
    return defaultIdentity();
  else
    return ident;
}

const Identity & IdentityManager::identityForUoidOrDefault( uint uoid ) const
{
  const Identity & ident = identityForUoid( uoid );
  if ( ident.isNull() )
    return defaultIdentity();
  else
    return ident;
}

const Identity & IdentityManager::identityForAddress( const QString & addresses ) const
{
  QStringList addressList = KPIM::splitEmailAddrList( addresses );
  for ( ConstIterator it = begin() ; it != end() ; ++it ) {
    for( QStringList::ConstIterator addrIt = addressList.begin();
         addrIt != addressList.end(); ++addrIt ) {
      // I use QString::utf8() instead of QString::latin1() because I want
      // a QCString and not a char*. It doesn't matter because emailAddr()
      // returns a 7-bit string.
      if( (*it).emailAddr().utf8().lower() ==
          KPIM::getEmailAddr( *addrIt ).lower() )
        return (*it);
    }
  }
  return Identity::null;
}

bool IdentityManager::thatIsMe( const QString & addressList ) const {
  return !identityForAddress( addressList ).isNull();
}

Identity & IdentityManager::identityForName( const QString & name )
{
  for ( Iterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) return (*it);
  kdWarning( 5006 ) << "IdentityManager::identityForName() used as newFromScratch() replacement!"
		    << "\n  name == \"" << name << "\"" << endl;
  return newFromScratch( name );
}

Identity & IdentityManager::identityForUoid( uint uoid )
{
  for ( Iterator it = begin() ; it != end() ; ++it )
    if ( (*it).uoid() == uoid ) return (*it);
  kdWarning( 5006 ) << "IdentityManager::identityForUoid() used as newFromScratch() replacement!"
		    << "\n  uoid == \"" << uoid << "\"" << endl;
  return newFromScratch( i18n("Unnamed") );
}

const Identity & IdentityManager::defaultIdentity() const {
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( (*it).isDefault() ) return (*it);
  (mIdentities.isEmpty() ? kdFatal( 5006 ) : kdWarning( 5006 ) )
    << "IdentityManager: No default identity found!" << endl;
  return *begin();
}

bool IdentityManager::setAsDefault( const QString & name ) {
  // First, check if the identity actually exists:
  QStringList names = shadowIdentities();
  if ( names.find( name ) == names.end() ) return false;
  // Then, change the default as requested:
  for ( Iterator it = begin() ; it != end() ; ++it )
    (*it).setIsDefault( (*it).identityName() == name );
  // and re-sort:
  sort();
  return true;
}

bool IdentityManager::setAsDefault( uint uoid ) {
  // First, check if the identity actually exists:
  bool found = false;
  for ( ConstIterator it = mShadowIdentities.begin() ;
	it != mShadowIdentities.end() ; ++it )
    if ( (*it).uoid() == uoid ) {
      found = true;
      break;
    }
  if ( !found ) return false;

  // Then, change the default as requested:
  for ( Iterator it = begin() ; it != end() ; ++it )
    (*it).setIsDefault( (*it).uoid() == uoid );
  // and re-sort:
  sort();
  return true;
}

bool IdentityManager::removeIdentity( const QString & name ) {
  for ( Iterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) {
      bool removedWasDefault = (*it).isDefault();
      mShadowIdentities.remove( it );
      if ( removedWasDefault )
	mShadowIdentities.first().setIsDefault( true );
      return true;
    }
  return false;
}

Identity & IdentityManager::newFromScratch( const QString & name ) {
  return newFromExisting( Identity( name ) );
}

Identity & IdentityManager::newFromControlCenter( const QString & name ) {
  KEMailSettings es;
  es.setProfile( es.defaultProfileName() );

  return newFromExisting( Identity( name,
			       es.getSetting( KEMailSettings::RealName ),
			       es.getSetting( KEMailSettings::EmailAddress ),
			       es.getSetting( KEMailSettings::Organization ),
			       es.getSetting( KEMailSettings::ReplyToAddress )
			       ) );
}

Identity & IdentityManager::newFromExisting( const Identity & other,
					       const QString & name ) {
  mShadowIdentities << other;
  Identity & result = mShadowIdentities.last();
  result.setIsDefault( false ); // we don't want two default identities!
  result.setUoid( newUoid() ); // we don't want two identies w/ same UOID
  if ( !name.isNull() )
    result.setIdentityName( name );
  return result;
}

void IdentityManager::createDefaultIdentity() {
  KUser user;
  QString fullName = user.fullName();

  QString emailAddress = user.loginName();
  if ( !emailAddress.isEmpty() ) {
    KConfigGroup general( mConfig, "General" );
    QString defaultdomain = general.readEntry( "Default domain" );
    if( !defaultdomain.isEmpty() ) {
      emailAddress += '@' + defaultdomain;
    }
    else {
      emailAddress = QString::null;
    }
  }
  // Let the application adjust those parameters
  createDefaultIdentity( fullName, emailAddress );
  mShadowIdentities << Identity( i18n("Default"), fullName, emailAddress );
  mShadowIdentities.last().setIsDefault( true );
  mShadowIdentities.last().setUoid( newUoid() );
  if ( mReadOnly ) // commit won't do it in readonly mode
    mIdentities = mShadowIdentities;
}

int IdentityManager::newUoid()
{
  int uoid;

  // determine the UOIDs of all saved identities
  QValueList<uint> usedUOIDs;
  for ( QValueList<Identity>::ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it )
    usedUOIDs << (*it).uoid();

  if ( hasPendingChanges() ) {
    // add UOIDs of all shadow identities. Yes, we will add a lot of duplicate
    // UOIDs, but avoiding duplicate UOIDs isn't worth the effort.
    for ( QValueList<Identity>::ConstIterator it = mShadowIdentities.begin() ;
          it != mShadowIdentities.end() ; ++it ) {
      usedUOIDs << (*it).uoid();
    }
  }

  usedUOIDs << 0; // no UOID must be 0 because this value always refers to the
                  // default identity

  do {
    uoid = kapp->random();
  } while ( usedUOIDs.find( uoid ) != usedUOIDs.end() );

  return uoid;
}

QStringList KPIM::IdentityManager::allEmails() const
{
  QStringList lst;
  for ( ConstIterator it = begin() ; it != end() ; ++it ) {
    lst << (*it).emailAddr();
  }
  return lst;
}

#include "identitymanager.moc"
