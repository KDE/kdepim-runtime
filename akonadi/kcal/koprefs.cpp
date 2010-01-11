/*
  This file is part of KOrganizer.

  Copyright (c) 2001,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "koprefs.h"
#include "identitymanager.h"

#include <kmime/kmime_header_parsing.h>
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include <kpimutils/email.h>
#include <libkdepim/kpimprefs.h>

#include <kglobalsettings.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <kemailsettings.h>
#include <k3staticdeleter.h>
#include <kstringhandler.h>
#include <ksystemtimezone.h>

#include <QDir>
#include <QString>
#include <QFont>
#include <QColor>
#include <QMap>
#include <QStringList>

#include <time.h>
#include <unistd.h>

using namespace KPIMIdentities;

K_GLOBAL_STATIC( KOPrefs, globalPrefs )

KOPrefs::KOPrefs() : KOPrefsBase()
{
  mDefaultAgendaTimeLabelsFont = KGlobalSettings::generalFont();
  // make a large default time bar font, at least 16 points.
  mDefaultAgendaTimeLabelsFont.setPointSize(
    qMax( mDefaultAgendaTimeLabelsFont.pointSize() + 4, 16 ) );

  mDefaultMonthViewFont = KGlobalSettings::generalFont();
  // make it a bit smaller
  mDefaultMonthViewFont.setPointSize(
    qMax( mDefaultMonthViewFont.pointSize() - 2, 6 ) );

  KConfigSkeleton::setCurrentGroup( QLatin1String( "General" ) );

  addItemPath( QLatin1String( "Html Export File" ), mHtmlExportFile,
      QDir::homePath() + QLatin1Char( '/' ) + i18nc( "Default export file", "calendar.html" ) );
}

KOPrefs::~KOPrefs()
{
  kDebug();
}

KOPrefs *KOPrefs::instance()
{
  static bool firstCall = true;

  if ( firstCall ) {
    firstCall = false;
    globalPrefs->readConfig();
  }

  return globalPrefs;
}

void KOPrefs::usrSetDefaults()
{
  // Default should be set a bit smarter, respecting username and locale
  // settings for example.

  KEMailSettings settings;
  QString tmp = settings.getSetting( KEMailSettings::RealName );
  if ( !tmp.isEmpty() ) {
    setUserName( tmp );
  }
  tmp = settings.getSetting( KEMailSettings::EmailAddress );
  if ( !tmp.isEmpty() ) {
    setUserEmail( tmp );
  }
  fillMailDefaults();

  setTimeZoneDefault();

  KConfigSkeleton::usrSetDefaults();
}

void KOPrefs::fillMailDefaults()
{
  userEmailItem()->swapDefault();
  QString defEmail = userEmailItem()->value();
  userEmailItem()->swapDefault();

  if ( userEmail() == defEmail ) {
    // No korg settings - but maybe there's a kcontrol[/kmail] setting available
    KEMailSettings settings;
    if ( !settings.getSetting( KEMailSettings::EmailAddress ).isEmpty() ) {
      mEmailControlCenter = true;
    }
  }
}

void KOPrefs::setTimeZoneDefault()
{
  KTimeZone zone = KSystemTimeZones::local();
  if ( !zone.isValid() ) {
    kError() << "KSystemTimeZones::local() return 0";
    return;
  }

  kDebug () << "----- time zone:" << zone.name();

  mTimeSpec = zone;
}

KDateTime::Spec KOPrefs::timeSpec()
{
  return KSystemTimeZones::local();
}

void KOPrefs::setTimeSpec( const KDateTime::Spec &spec )
{
  mTimeSpec = spec;
}

void KOPrefs::usrReadConfig()
{
  KConfigGroup generalConfig( config(), "General" );
  mMailTransport = generalConfig.readEntry( "MailTransport", QString() );

  if ( !mTimeSpec.isValid() ) {
    setTimeZoneDefault();
  }

#if 0
  config()->setGroup( "FreeBusy" );
  if ( mRememberRetrievePw ) {
    mRetrievePassword =
      KStringHandler::obscure( config()->readEntry( "Retrieve Server Password" ) );
  }
#endif
  KConfigGroup timeScaleConfig( config(), "Timescale" );
  setTimeScaleTimezones( timeScaleConfig.readEntry( "Timescale Timezones", QStringList() ) );

  KConfigSkeleton::usrReadConfig();
  fillMailDefaults();
}

void KOPrefs::usrWriteConfig()
{
  KConfigGroup generalConfig( config(), "General" );
  if( ! mMailTransport.isNull() )
    generalConfig.writeEntry( "MailTransport", mMailTransport );

  if ( !mFreeBusyPublishSavePassword ) {
    KConfigSkeleton::ItemPassword *i = freeBusyPublishPasswordItem();
    i->setValue( QString() );
    i->writeConfig( config() );
  }
  if ( !mFreeBusyRetrieveSavePassword ) {
    KConfigSkeleton::ItemPassword *i = freeBusyRetrievePasswordItem();
    i->setValue( QString() );
    i->writeConfig( config() );
  }

#if 0
  if ( mRememberRetrievePw ) {
    config()->writeEntry( "Retrieve Server Password",
                          KStringHandler::obscure( mRetrievePassword ) );
  } else {
    config()->deleteEntry( "Retrieve Server Password" );
  }
#endif

  KConfigGroup timeScaleConfig( config(), QLatin1String( "Timescale" ) );
  timeScaleConfig.writeEntry( QLatin1String( "Timescale Timezones" ), timeScaleTimezones() );

  KConfigSkeleton::usrWriteConfig();
}

QString KOPrefs::fullName()
{
  QString tusername;
  if ( mEmailControlCenter ) {
    KEMailSettings settings;
    tusername = settings.getSetting( KEMailSettings::RealName );
  } else {
    tusername = userName();
  }

  // Quote the username as it might contain commas and other quotable chars.
  tusername = KPIMUtils::quoteNameIfNecessary( tusername );

  QString tname, temail;
  // ignore the return value from extractEmailAddressAndName() because
  // it will always be false since tusername does not contain "@domain".
  KPIMUtils::extractEmailAddressAndName( tusername, temail, tname );
  return tname;
}

QString KOPrefs::email()
{
  if ( mEmailControlCenter ) {
    KEMailSettings settings;
    return settings.getSetting( KEMailSettings::EmailAddress );
  } else {
    return userEmail();
  }
}

QStringList KOPrefs::allEmails()
{
  // Grab emails from the email identities
  QStringList lst = Akonadi::identityManager()->allEmails();
  // Add emails configured in korganizer
  lst += mAdditionalMails;
  // Add the email entered as the userEmail here
  lst += email();

  // Warning, this list could contain duplicates.
  return lst;
}

QStringList KOPrefs::fullEmails()
{
  QStringList fullEmails;
  // The user name and email from the config dialog:
  fullEmails << QString::fromUtf8( "%1 <%2>" ).arg( fullName() ).arg( email() );

  QStringList::Iterator it;
  // Grab emails from the email identities
  KPIMIdentities::IdentityManager *idmanager = Akonadi::identityManager();
  QStringList lst = idmanager->identities();
  KPIMIdentities::IdentityManager::ConstIterator it1;
  for ( it1 = idmanager->begin(); it1 != idmanager->end(); ++it1 ) {
    fullEmails << (*it1).fullEmailAddr();
  }
  // Add emails configured in korganizer
  lst = mAdditionalMails;
  for ( it = lst.begin(); it != lst.end(); ++it ) {
    fullEmails << QString::fromUtf8( "%1 <%2>" ).arg( fullName() ).arg( *it );
  }

  // Warning, this list could contain duplicates.
  return fullEmails;
}

bool KOPrefs::thatIsMe( const QString &_email )
{
  // NOTE: this method is called for every created agenda view item,
  // so we need to keep performance in mind

  /* identityManager()->thatIsMe() is quite expensive since it does parsing of
     _email in a way which is unnecessarily complex for what we can have here,
     so we do that ourselves. This makes sense since this

  if ( Akonadi::identityManager()->thatIsMe( _email ) ) {
    return true;
  }
  */

  // in case email contains a full name, strip it out.
  // the below is the simpler but slower version of the following code:
  // const QString email = KPIM::getEmailAddress( _email );
  const QByteArray tmp = _email.toUtf8();
  const char *cursor = tmp.constData();
  const char *end = tmp.data() + tmp.length();
  KMime::Types::Mailbox mbox;
  KMime::HeaderParsing::parseMailbox( cursor, end, mbox );
  const QString email = mbox.addrSpec().asString();

  if ( this->email() == email ) {
    return true;
  }

  for ( Akonadi::IdentityManager::ConstIterator it = Akonadi::identityManager()->begin();
        it != Akonadi::identityManager()->end(); ++it ) {
    if ( email == (*it).emailAddr() ) {
      return true;
    }
  }

  if ( mAdditionalMails.contains( email ) ) {
    return true;
  }

  return false;
}

QStringList KOPrefs::timeScaleTimezones()
{
  return mTimeScaleTimeZones;
}

void KOPrefs::setTimeScaleTimezones( const QStringList &list )
{
  mTimeScaleTimeZones = list;
}
