/* kldapclient.cpp - LDAP access
 *      Copyright (C) 2002 Klarälvdalens Datakonsult AB
 *
 *      Author: Steffen Hansen <hansen@kde.org>
 *
 *      Ported to KABC by Daniel Molkentin <molkentin@kde.org>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "ldapclient.h"

#include <kldap/ldapurl.h>
#include <kldap/ldif.h>

#include <KCodecs>
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KDirWatch>
#include <KProtocolInfo>
#include <KStandardDirs>
#include <K3StaticDeleter>

#include <QFile>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QTextStream>

using namespace KPIM;

KConfig *KPIM::LdapSearch::s_config = 0L;
static K3StaticDeleter<KConfig> configDeleter;

LdapClient::LdapClient( int clientNumber, QObject* parent, const char* name )
  : QObject( parent ), mJob( 0 ), mActive( false )
{
//  d = new LdapClientPrivate;
  setObjectName(name);
  mClientNumber = clientNumber;
  mCompletionWeight = 50 - mClientNumber;
}

LdapClient::~LdapClient()
{
  cancelQuery();
//  delete d; d = 0;
}

void LdapClient::setAttrs( const QStringList& attrs )
{
  mAttrs = attrs;
}

void LdapClient::startQuery( const QString& filter )
{
  cancelQuery();
  KLDAP::LdapUrl url;

  url = mServer.url();

  url.setAttributes( mAttrs );
  url.setScope( mScope == "one" ? KLDAP::LdapUrl::One : KLDAP::LdapUrl::Sub );
  url.setFilter( '('+filter+')' );

  kDebug(5300) <<"LdapClient: Doing query:" << url.prettyUrl();

  startParseLDIF();
  mActive = true;
  mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
  connect( mJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
           this, SLOT( slotData( KIO::Job*, const QByteArray& ) ) );
  connect( mJob, SIGNAL( infoMessage( KJob*, const QString&, const QString& ) ),
           this, SLOT( slotInfoMessage( KJob*, const QString&, const QString& ) ) );
  connect( mJob, SIGNAL( result( KJob* ) ),
           this, SLOT( slotDone() ) );
}

void LdapClient::cancelQuery()
{
  if ( mJob ) {
    mJob->kill();
    mJob = 0;
  }

  mActive = false;
}

void LdapClient::slotData( KIO::Job*, const QByteArray& data )
{
  parseLDIF( data );
}

void LdapClient::slotInfoMessage( KJob*, const QString &, const QString& )
{
  //qDebug("Job said \"%s\"", info.toLatin1());
}

void LdapClient::slotDone()
{
  endParseLDIF();
  mActive = false;
#if 0
  for ( Q3ValueList<LdapObject>::Iterator it = mObjects.begin(); it != mObjects.end(); ++it ) {
    qDebug( (*it).toString().toLatin1() );
  }
#endif
  int err = mJob->error();
  if ( err && err != KIO::ERR_USER_CANCELED ) {
    emit error( mJob->errorString() );
  }
  emit done();
}

void LdapClient::startParseLDIF()
{
  mCurrentObject.clear();
  mLdif.startParsing();
}

void LdapClient::endParseLDIF()
{
}

void LdapClient::finishCurrentObject()
{
  mCurrentObject.setDn( mLdif.dn() );
  KLDAP::LdapAttrValue objectclasses;
  for ( KLDAP::LdapAttrMap::ConstIterator it = mCurrentObject.attributes().begin();
    it != mCurrentObject.attributes().end(); ++it ) {

    if ( it.key().toLower() == "objectclass" ) {
      objectclasses = it.value();
      break;
    }
  }

  bool groupofnames = false;
  for ( KLDAP::LdapAttrValue::ConstIterator it = objectclasses.begin();
    it != objectclasses.end(); ++it ) {

    QByteArray sClass = (*it).toLower();
    if ( sClass == "groupofnames" || sClass == "kolabgroupofnames" ) {
      groupofnames = true;
    }
  }

  if( groupofnames ){
    KLDAP::LdapAttrMap::ConstIterator it = mCurrentObject.attributes().find("mail");
    if( it == mCurrentObject.attributes().end() ){
      // No explicit mail address found so far?
      // Fine, then we use the address stored in the DN.
      QString sMail;
      QStringList lMail = mCurrentObject.dn().toString().split(",dc=", QString::SkipEmptyParts);
      const int n = lMail.count();
      if( n ){
        if( lMail.first().toLower().startsWith("cn=") ){
          sMail = lMail.first().simplified().mid(3);
          if( 1 < n )
            sMail.append('@');
          for( int i=1; i<n; ++i){
            sMail.append( lMail[i] );
            if( i < n-1 )
              sMail.append('.');
          }
          mCurrentObject.addValue("mail", sMail.toUtf8() );
        }
      }
    }
  }
  emit result( *this, mCurrentObject );
  mCurrentObject.clear();
}

void LdapClient::parseLDIF( const QByteArray& data )
{
  //kDebug(5300) <<"LdapClient::parseLDIF(" << QCString(data.data(), data.size()+1) <<" )";
  if ( data.size() ) {
    mLdif.setLdif( data );
  } else {
    mLdif.endLdif();
  }
  KLDAP::Ldif::ParseValue ret;
  QString name;
  do {
    ret = mLdif.nextItem();
    switch ( ret ) {
      case KLDAP::Ldif::Item:
        {
          name = mLdif.attr();
          QByteArray value = mLdif.value();
          mCurrentObject.addValue( name, value );
          //kDebug(5300) <<"LdapClient::parseLDIF(): name=" << name <<" value=" << QCString(value.data(), value.size()+1);
        }
        break;
     case KLDAP::Ldif::EndEntry:
        finishCurrentObject();
        break;
      default:
        break;
    }
  } while ( ret != KLDAP::Ldif::MoreData );
}

int LdapClient::clientNumber() const
{
  return mClientNumber;
}

int LdapClient::completionWeight() const
{
  return mCompletionWeight;
}

void LdapClient::setCompletionWeight( int weight )
{
  mCompletionWeight = weight;
}

void LdapSearch::readConfig( KLDAP::LdapServer &server, const KConfigGroup &config, int j, bool active )
{
  QString prefix;
  if ( active ) prefix = "Selected";
  QString host =  config.readEntry( prefix + QString( "Host%1" ).arg( j ), QString() ).trimmed();
  if ( !host.isEmpty() )
    server.setHost( host );

  int port = config.readEntry( prefix + QString( "Port%1" ).arg( j ), 389 );
  server.setPort( port );

  QString base = config.readEntry( prefix + QString( "Base%1" ).arg( j ), QString() ).trimmed();
  if ( !base.isEmpty() )
    server.setBaseDn( KLDAP::LdapDN( base ) );

  QString user = config.readEntry( prefix + QString( "User%1" ).arg( j ), QString() ).trimmed();
  if ( !user.isEmpty() )
    server.setUser( user );

  QString bindDN = config.readEntry( prefix + QString( "Bind%1" ).arg( j ), QString() ).trimmed();
  if ( !bindDN.isEmpty() )
    server.setBindDn( bindDN );

  QString pwdBindDN = config.readEntry( prefix + QString( "PwdBind%1" ).arg( j ), QString() );
  if ( !pwdBindDN.isEmpty() )
    server.setPassword( pwdBindDN );

  server.setTimeLimit( config.readEntry( prefix + QString( "TimeLimit%1" ).arg( j ),0 ) );
  server.setSizeLimit( config.readEntry( prefix + QString( "SizeLimit%1" ).arg( j ),0 ) );
  server.setPageSize( config.readEntry( prefix + QString( "PageSize%1" ).arg( j ),0 ) );
  server.setVersion( config.readEntry( prefix + QString( "Version%1" ).arg( j ), 3 ) );

  QString tmp;
  tmp = config.readEntry( prefix + QString( "Security%1" ).arg( j ), QString::fromLatin1("None") );
  server.setSecurity( KLDAP::LdapServer::None );
  if ( tmp == "SSL" ) server.setSecurity( KLDAP::LdapServer::SSL );
  else if ( tmp == "TLS" ) server.setSecurity( KLDAP::LdapServer::TLS );

  tmp = config.readEntry( prefix + QString( "Auth%1" ).arg( j ), QString::fromLatin1("Anonymous") );
  server.setAuth( KLDAP::LdapServer::Anonymous );
  if ( tmp == "Simple" ) server.setAuth( KLDAP::LdapServer::Simple );
  else if ( tmp == "SASL" ) server.setAuth( KLDAP::LdapServer::SASL );

  server.setMech( config.readEntry( prefix + QString( "Mech%1" ).arg( j ), QString() ) );
}

void LdapSearch::writeConfig( const KLDAP::LdapServer &server, KConfigGroup &config, int j, bool active )
{
  QString prefix;
  if ( active ) prefix = "Selected";
  config.writeEntry( prefix + QString( "Host%1" ).arg( j ), server.host() );
  config.writeEntry( prefix + QString( "Port%1" ).arg( j ), server.port() );
  config.writeEntry( prefix + QString( "Base%1" ).arg( j ), server.baseDn().toString() );
  config.writeEntry( prefix + QString( "User%1" ).arg( j ), server.user() );
  config.writeEntry( prefix + QString( "Bind%1" ).arg( j ), server.bindDn() );
  config.writeEntry( prefix + QString( "PwdBind%1" ).arg( j ), server.password() );
  config.writeEntry( prefix + QString( "TimeLimit%1" ).arg( j ), server.timeLimit() );
  config.writeEntry( prefix + QString( "SizeLimit%1" ).arg( j ), server.sizeLimit() );
  config.writeEntry( prefix + QString( "PageSize%1" ).arg( j ), server.pageSize() );
  config.writeEntry( prefix + QString( "Version%1" ).arg( j ), server.version() );
  QString tmp;
  switch ( server.security() ) {
    case KLDAP::LdapServer::TLS:
      tmp = "TLS";
      break;
    case KLDAP::LdapServer::SSL:
      tmp = "SSL";
      break;
    default:
      tmp = "None";
  }
  config.writeEntry( prefix + QString( "Security%1" ).arg( j ), tmp );
  switch ( server.auth() ) {
    case KLDAP::LdapServer::Simple:
      tmp = "Simple";
      break;
    case KLDAP::LdapServer::SSL:
      tmp = "SASL";
      break;
    default:
      tmp = "Anonymous";
  }
  config.writeEntry( prefix + QString( "Auth%1" ).arg( j ), tmp );
  config.writeEntry( prefix + QString( "Mech%1" ).arg( j ), server.mech() );
}

KConfig* LdapSearch::config()
{
  if ( !s_config )
    configDeleter.setObject( s_config, new KConfig("kabldaprc", KConfig::NoGlobals) ); // Open read-write, no kdeglobals

  return s_config;
}


LdapSearch::LdapSearch()
    : mActiveClients( 0 ), mNoLDAPLookup( false )
{
  if ( !KProtocolInfo::isKnownProtocol( KUrl("ldap://localhost") ) ) {
    mNoLDAPLookup = true;
    return;
  }

  readConfig();
  connect(KDirWatch::self(), SIGNAL(dirty (const QString&)),this,
          SLOT(slotFileChanged(const QString&)));
}

void LdapSearch::readConfig()
{
  cancelSearch();
  QList< LdapClient* >::Iterator it;
  for ( it = mClients.begin(); it != mClients.end(); ++it )
    delete *it;
  mClients.clear();

  // stolen from KAddressBook
  KConfigGroup config(KPIM::LdapSearch::config(), "LDAP" );
  int numHosts = config.readEntry( "NumSelectedHosts",0);
  if ( !numHosts ) {
    mNoLDAPLookup = true;
  } else {
    for ( int j = 0; j < numHosts; j++ ) {
      LdapClient* ldapClient = new LdapClient( j, this );
      KLDAP::LdapServer server;
      readConfig( server, config, j, true );
      if ( !server.host().isEmpty() ) mNoLDAPLookup = false;
      ldapClient->setServer( server );

      int completionWeight = config.readEntry( QString( "SelectedCompletionWeight%1" ).arg( j ), -1 );
      if ( completionWeight != -1 )
        ldapClient->setCompletionWeight( completionWeight );

      QStringList attrs;
      // note: we need "objectClass" to detect distribution lists
      attrs << "cn" << "mail" << "givenname" << "sn" << "objectClass";
      ldapClient->setAttrs( attrs );

      connect( ldapClient, SIGNAL( result( const KPIM::LdapClient&, const KLDAP::LdapObject& ) ),
               this, SLOT( slotLDAPResult( const KPIM::LdapClient&, const KLDAP::LdapObject& ) ) );
      connect( ldapClient, SIGNAL( done() ),
               this, SLOT( slotLDAPDone() ) );
      connect( ldapClient, SIGNAL( error( const QString& ) ),
               this, SLOT( slotLDAPError( const QString& ) ) );

      mClients.append( ldapClient );
    }

    connect( &mDataTimer, SIGNAL( timeout() ), SLOT( slotDataTimer() ) );
  }
  mConfigFile = KStandardDirs::locateLocal( "config", "kabldaprc" );
  KDirWatch::self()->addFile( mConfigFile );
}

void LdapSearch::slotFileChanged( const QString& file )
{
  if ( file == mConfigFile )
    readConfig();
}

void LdapSearch::startSearch( const QString& txt )
{
  if ( mNoLDAPLookup )
    return;

  cancelSearch();

  int pos = txt.indexOf( '\"' );
  if( pos >= 0 )
  {
    ++pos;
    int pos2 = txt.indexOf( '\"', pos );
    if( pos2 >= 0 )
        mSearchText = txt.mid( pos , pos2 - pos );
    else
        mSearchText = txt.mid( pos );
  } else
    mSearchText = txt;

  /* The reasoning behind this filter is:
   * If it's a person, or a distlist, show it, even if it doesn't have an email address.
   * If it's not a person, or a distlist, only show it if it has an email attribute.
   * This allows both resource accounts with an email address which are not a person and
   * person entries without an email address to show up, while still not showing things
   * like structural entries in the ldap tree. */
  QString filter = QString( "&(|(objectclass=person)(objectclass=groupOfNames)(mail=*))(|(cn=%1*)(mail=%2*)(mail=*@%3*)(givenName=%4*)(sn=%5*))" )
      .arg( mSearchText ).arg( mSearchText ).arg( mSearchText ).arg( mSearchText ).arg( mSearchText );

  QList< LdapClient* >::Iterator it;
  for ( it = mClients.begin(); it != mClients.end(); ++it ) {
    (*it)->startQuery( filter );
    kDebug(5300) <<"LdapSearch::startSearch()" << filter;
    ++mActiveClients;
  }
}

void LdapSearch::cancelSearch()
{
  QList< LdapClient* >::Iterator it;
  for ( it = mClients.begin(); it != mClients.end(); ++it )
    (*it)->cancelQuery();

  mActiveClients = 0;
  mResults.clear();
}

void LdapSearch::slotLDAPResult( const LdapClient &client, const KLDAP::LdapObject& obj )
{
  ResultObject result;
  result.client = &client;
  result.object = obj;

  mResults.append( result );
  if ( !mDataTimer.isActive() )
  {
    mDataTimer.setSingleShot( true );
    mDataTimer.start( 500 );
  }
}

void LdapSearch::slotLDAPError( const QString& )
{
  slotLDAPDone();
}

void LdapSearch::slotLDAPDone()
{
  if ( --mActiveClients > 0 )
    return;

  finish();
}

void LdapSearch::slotDataTimer()
{
  QStringList lst;
  LdapResultList reslist;
  makeSearchData( lst, reslist );
  if ( !lst.isEmpty() )
    emit searchData( lst );
  if ( !reslist.isEmpty() )
    emit searchData( reslist );
}

void LdapSearch::finish()
{
  mDataTimer.stop();

  slotDataTimer(); // emit final bunch of data
  emit searchDone();
}

void LdapSearch::makeSearchData( QStringList& ret, LdapResultList& resList )
{
  QString search_text_upper = mSearchText.toUpper();

  QList< ResultObject >::ConstIterator it1;
  for ( it1 = mResults.begin(); it1 != mResults.end(); ++it1 ) {
    QString name, mail, givenname, sn;
    QStringList mails;
    bool isDistributionList = false;
    bool wasCN = false;
    bool wasDC = false;

    kDebug(5300) <<"\n\nLdapSearch::makeSearchData()";

    KLDAP::LdapAttrMap::ConstIterator it2;
    for ( it2 = (*it1).object.attributes().begin(); it2 != (*it1).object.attributes().end(); ++it2 ) {
      QByteArray val = (*it2).first();
      int len = val.size();
      if( len > 0 && '\0' == val[len-1] )
        --len;
      const QString tmp = QString::fromUtf8( val, len );
      kDebug(5300) <<"      key: \"" << it2.key() <<"\" value: \"" << tmp <<"\"";
      if ( it2.key() == "cn" ) {
        name = tmp;
        if( mail.isEmpty() )
          mail = tmp;
        else{
          if( wasCN )
            mail.prepend( "." );
          else
            mail.prepend( "@" );
          mail.prepend( tmp );
        }
        wasCN = true;
      } else if ( it2.key() == "dc" ) {
        if( mail.isEmpty() )
          mail = tmp;
        else{
          if( wasDC )
            mail.append( "." );
          else
            mail.append( "@" );
          mail.append( tmp );
        }
        wasDC = true;
      } else if( it2.key() == "mail" ) {
        mail = tmp;
        KLDAP::LdapAttrValue::ConstIterator it3 = it2.value().begin();
        for ( ; it3 != it2.value().end(); ++it3 ) {
          mails.append( QString::fromUtf8( (*it3).data(), (*it3).size() ) );
        }
      } else if( it2.key() == "givenName" )
        givenname = tmp;
      else if( it2.key() == "sn" )
        sn = tmp;
      else if( it2.key() == "objectClass" &&
               (tmp == "groupOfNames" || tmp == "kolabGroupOfNames") ) {
        isDistributionList = true;
      }
    }

    if( mails.isEmpty()) {
      if ( !mail.isEmpty() ) mails.append( mail );
      if( isDistributionList ) {
        kDebug(5300) <<"\n\nLdapSearch::makeSearchData() found a list:" << name;
        ret.append( name );
        // following lines commented out for bugfixing kolab issue #177:
        //
        // Unlike we thought previously we may NOT append the server name here.
        //
        // The right server is found by the SMTP server instead: Kolab users
        // must use the correct SMTP server, by definition.
        //
        //mail = (*it1).client->base().simplified();
        //mail.replace( ",dc=", ".", false );
        //if( mail.startsWith("dc=", false) )
        //  mail.remove(0, 3);
        //mail.prepend( '@' );
        //mail.prepend( name );
        //mail = name;
      } else {
        kDebug(5300) <<"LdapSearch::makeSearchData() found BAD ENTRY: \"" << name <<"\"";
        continue; // nothing, bad entry
      }
    } else if ( name.isEmpty() ) {
      kDebug(5300) <<"LdapSearch::makeSearchData() mail: \"" << mail <<"\"";
      ret.append( mail );
    } else {
      kDebug(5300) <<"LdapSearch::makeSearchData() name: \"" << name <<"\"  mail: \"" << mail <<"\"";
      ret.append( QString( "%1 <%2>" ).arg( name ).arg( mail ) );
    }

    LdapResult sr;
    sr.clientNumber = (*it1).client->clientNumber();
    sr.completionWeight = (*it1).client->completionWeight();
    sr.name = name;
    sr.email = mails;
    resList.append( sr );
  }

  mResults.clear();
}

bool LdapSearch::isAvailable() const
{
  return !mNoLDAPLookup;
}


#include "ldapclient.moc"
