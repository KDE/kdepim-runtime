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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */



#include <qfile.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qtextstream.h>
#include <qurl.h>

#include <kabc/ldapurl.h>
#include <kabc/ldif.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <kmdcodec.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>

#include "ldapclient.h"

using namespace KPIM;

class LdapClient::LdapClientPrivate{
public:
  QString bindDN;
  QString pwdBindDN;
  KABC::LDIF ldif;
  int clientNumber;
  int completionWeight;
};

QString LdapObject::toString() const
{
  QString result = QString::fromLatin1( "\ndn: %1\n" ).arg( dn );
  for ( LdapAttrMap::ConstIterator it = attrs.begin(); it != attrs.end(); ++it ) {
    QString attr = it.key();
    for ( LdapAttrValue::ConstIterator it2 = (*it).begin(); it2 != (*it).end(); ++it2 ) {
      result += QString::fromUtf8( KABC::LDIF::assembleLine( attr, *it2, 76 ) ) + "\n";
    }
  }

  return result;
}

void LdapObject::clear()
{
  dn = QString::null;
  objectClass = QString::null;
  attrs.clear();
}

void LdapObject::assign( const LdapObject& that )
{
  if ( &that != this ) {
    dn = that.dn;
    attrs = that.attrs;
    client = that.client;
  }
}

LdapClient::LdapClient( int clientNumber, QObject* parent, const char* name )
  : QObject( parent, name ), mJob( 0 ), mActive( false ), mReportObjectClass( false )
{
  d = new LdapClientPrivate;
  d->clientNumber = clientNumber;
  d->completionWeight = 50 - clientNumber;
}

LdapClient::~LdapClient()
{
  cancelQuery();
  delete d; d = 0;
}

void LdapClient::setHost( const QString& host )
{
  mHost = host;
}

void LdapClient::setPort( const QString& port )
{
  mPort = port;
}

void LdapClient::setBase( const QString& base )
{
  mBase = base;
}

void LdapClient::setBindDN( const QString& bindDN )
{
  d->bindDN = bindDN;
}

void LdapClient::setPwdBindDN( const QString& pwdBindDN )
{
  d->pwdBindDN = pwdBindDN;
}

void LdapClient::setAttrs( const QStringList& attrs )
{
  mAttrs = attrs;
  for ( QStringList::Iterator it = mAttrs.begin(); it != mAttrs.end(); ++it )
    if( (*it).lower() == "objectclass" ){
      mReportObjectClass = true;
      return;
    }
  mAttrs << "objectClass"; // via objectClass we detect distribution lists
  mReportObjectClass = false;
}

void LdapClient::startQuery( const QString& filter )
{
  cancelQuery();
  KABC::LDAPUrl url;

  url.setProtocol( "ldap" );
  url.setUser( d->bindDN );
  url.setPass( d->pwdBindDN );
  url.setHost( mHost );
  url.setPort( mPort.toUInt() );
  url.setDn( mBase );
  url.setAttributes( mAttrs );
  url.setScope( mScope == "one" ? KABC::LDAPUrl::One : KABC::LDAPUrl::Sub );
  url.setFilter( "("+filter+")" );

  kdDebug(5300) << "LdapClient: Doing query: " << url.prettyURL() << endl;

  startParseLDIF();
  mActive = true;
  mJob = KIO::get( url, false, false );
  connect( mJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
           this, SLOT( slotData( KIO::Job*, const QByteArray& ) ) );
  connect( mJob, SIGNAL( infoMessage( KIO::Job*, const QString& ) ),
           this, SLOT( slotInfoMessage( KIO::Job*, const QString& ) ) );
  connect( mJob, SIGNAL( result( KIO::Job* ) ),
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
#ifndef NDEBUG // don't create the QString
//  QString str( data );
//  kdDebug(5700) << "LdapClient: Got \"" << str << "\"\n";
#endif
  parseLDIF( data );
}

void LdapClient::slotInfoMessage( KIO::Job*, const QString & )
{
  //qDebug("Job said \"%s\"", info.latin1());
}

void LdapClient::slotDone()
{
  endParseLDIF();
  mActive = false;
#if 0
  for ( QValueList<LdapObject>::Iterator it = mObjects.begin(); it != mObjects.end(); ++it ) {
    qDebug( (*it).toString().latin1() );
  }
#endif
  int err = mJob->error();
  if ( err && err != KIO::ERR_USER_CANCELED ) {
    emit error( KIO::buildErrorString( err, QString("%1:%2").arg( mHost ).arg( mPort ) ) );
  }
  emit done();
}

void LdapClient::startParseLDIF()
{
  mCurrentObject.clear();
  mLastAttrName  = 0;
  mLastAttrValue = 0;
  mIsBase64 = false;
  d->ldif.startParsing();
}

void LdapClient::endParseLDIF()
{
}

void LdapClient::finishCurrentObject()
{
  mCurrentObject.dn = d->ldif.dn();
  const QString sClass( mCurrentObject.objectClass.lower() );
  if( sClass == "groupofnames" || sClass == "kolabgroupofnames" ){
    LdapAttrMap::ConstIterator it = mCurrentObject.attrs.find("mail");
    if( it == mCurrentObject.attrs.end() ){
      // No explicit mail address found so far?
      // Fine, then we use the address stored in the DN.
      QString sMail;
      QStringList lMail = QStringList::split(",dc=", mCurrentObject.dn);
      const int n = lMail.count();
      if( n ){
        if( lMail.first().lower().startsWith("cn=") ){
          sMail = lMail.first().simplifyWhiteSpace().mid(3);
          if( 1 < n )
            sMail.append('@');
          for( int i=1; i<n; ++i){
            sMail.append( lMail[i] );
            if( i < n-1 )
              sMail.append('.');
          }
          mCurrentObject.attrs["mail"].append( sMail.utf8() );
        }
      }
    }
  }
  mCurrentObject.client = this;
  emit result( mCurrentObject );
  mCurrentObject.clear();
}

void LdapClient::parseLDIF( const QByteArray& data )
{
  //kdDebug(5300) << "LdapClient::parseLDIF( " << QCString(data) << " )" << endl;
  if ( data.size() ) {
    d->ldif.setLDIF( data );
  } else {
    d->ldif.endLDIF();
  }

  KABC::LDIF::ParseVal ret;
  QString name;
  QByteArray value;
  do {
    ret = d->ldif.nextItem();
    switch ( ret ) {
      case KABC::LDIF::Item: 
        {
          name = d->ldif.attr();
          value = d->ldif.val();
          bool bFoundOC = name.lower() == "objectclass";
          if( bFoundOC )
            mCurrentObject.objectClass = QString::fromUtf8( value, value.size() );
          if( mReportObjectClass || !bFoundOC )
            mCurrentObject.attrs[ name ].append( value );
          //kdDebug(5300) << "LdapClient::parseLDIF()" << name << " / " << value << endl;
        }
        break;
     case KABC::LDIF::EndEntry:
        finishCurrentObject();
        break;
      default:
        break;
    }
  } while ( ret != KABC::LDIF::MoreData );
}

QString LdapClient::bindDN() const
{
  return d->bindDN;
}

QString LdapClient::pwdBindDN() const
{
  return d->pwdBindDN;
}

int LdapClient::clientNumber() const
{
  return d->clientNumber;
}

int LdapClient::completionWeight() const
{
  return d->completionWeight;
}

void KPIM::LdapClient::setCompletionWeight( int weight )
{
  d->completionWeight = weight;
}

LdapSearch::LdapSearch()
    : mActiveClients( 0 ), mNoLDAPLookup( false )
{
  if ( !KProtocolInfo::isKnownProtocol( KURL("ldap://localhost") ) ) {
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
  QValueList< LdapClient* >::Iterator it;
  for ( it = mClients.begin(); it != mClients.end(); ++it )
    delete *it;
  mClients.clear();

  // stolen from KAddressBook
  KConfig config( "kabldaprc", true );
  config.setGroup( "LDAP" );
  int numHosts = config.readUnsignedNumEntry( "NumSelectedHosts");
  if ( !numHosts ) {
    mNoLDAPLookup = true;
  } else {
    for ( int j = 0; j < numHosts; j++ ) {
      LdapClient* ldapClient = new LdapClient( j, this );

      QString host =  config.readEntry( QString( "SelectedHost%1" ).arg( j ), "" ).stripWhiteSpace();
      if ( !host.isEmpty() ){
        ldapClient->setHost( host );
        mNoLDAPLookup = false;
      }

      QString port = QString::number( config.readUnsignedNumEntry( QString( "SelectedPort%1" ).arg( j ) ) );
      if ( !port.isEmpty() )
        ldapClient->setPort( port );

      QString base = config.readEntry( QString( "SelectedBase%1" ).arg( j ), "" ).stripWhiteSpace();
      if ( !base.isEmpty() )
        ldapClient->setBase( base );

      QString bindDN = config.readEntry( QString( "SelectedBind%1" ).arg( j ) ).stripWhiteSpace();
      if ( !bindDN.isEmpty() )
        ldapClient->setBindDN( bindDN );

      QString pwdBindDN = config.readEntry( QString( "SelectedPwdBind%1" ).arg( j ) ).stripWhiteSpace();
      if ( !pwdBindDN.isEmpty() )
        ldapClient->setPwdBindDN( pwdBindDN );

      int completionWeight = config.readNumEntry( QString( "SelectedCompletionWeight%1" ).arg( j ), -1 );
      if ( completionWeight != -1 )
        ldapClient->setCompletionWeight( completionWeight );

      QStringList attrs;
      // note: we need "objectClass" to detect distribution lists
      attrs << "cn" << "mail" << "givenname" << "sn" << "objectClass";
      ldapClient->setAttrs( attrs );

      connect( ldapClient, SIGNAL( result( const KPIM::LdapObject& ) ),
               this, SLOT( slotLDAPResult( const KPIM::LdapObject& ) ) );
      connect( ldapClient, SIGNAL( done() ),
               this, SLOT( slotLDAPDone() ) );
      connect( ldapClient, SIGNAL( error( const QString& ) ),
               this, SLOT( slotLDAPError( const QString& ) ) );

      mClients.append( ldapClient );
    }

    connect( &mDataTimer, SIGNAL( timeout() ), SLOT( slotDataTimer() ) );
  }
  mConfigFile = locateLocal( "config", "kabldaprc" );
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

  int pos = txt.find( '\"' );
  if( pos >= 0 )
  {
    ++pos;
    int pos2 = txt.find( '\"', pos );
    if( pos2 >= 0 )
        mSearchText = txt.mid( pos , pos2 - pos );
    else
        mSearchText = txt.mid( pos );
  } else
    mSearchText = txt;

  QString filter = QString( "|(cn=%1*)(mail=%2*)(givenName=%3*)(sn=%4*)" )
      .arg( mSearchText ).arg( mSearchText ).arg( mSearchText ).arg( mSearchText );

  QValueList< LdapClient* >::Iterator it;
  for ( it = mClients.begin(); it != mClients.end(); ++it ) {
    (*it)->startQuery( filter );
    kdDebug(5300) << "LdapSearch::startSearch() " << filter << endl;
    ++mActiveClients;
  }
}

void LdapSearch::cancelSearch()
{
  QValueList< LdapClient* >::Iterator it;
  for ( it = mClients.begin(); it != mClients.end(); ++it )
    (*it)->cancelQuery();

  mActiveClients = 0;
  mResults.clear();
}

void LdapSearch::slotLDAPResult( const KPIM::LdapObject& obj )
{
  mResults.append( obj );
  if ( !mDataTimer.isActive() )
    mDataTimer.start( 500, true );
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
  QString search_text_upper = mSearchText.upper();

  QValueList< KPIM::LdapObject >::ConstIterator it1;
  for ( it1 = mResults.begin(); it1 != mResults.end(); ++it1 ) {
    QString name, mail, givenname, sn;
    QStringList mails;
    bool isDistributionList = false;
    bool wasCN = false;
    bool wasDC = false;

    kdDebug(5300) << "\n\nLdapSearch::makeSearchData()\n\n" << endl;

    LdapAttrMap::ConstIterator it2;
    for ( it2 = (*it1).attrs.begin(); it2 != (*it1).attrs.end(); ++it2 ) {
      QByteArray val = (*it2).first();
      int len = val.size();
      if( '\0' == val[len-1] )
        --len;
      const QString tmp = QString::fromUtf8( val, len );
      kdDebug(5300) << "      key: \"" << it2.key() << "\" value: \"" << tmp << "\"" << endl;
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
        LdapAttrValue::ConstIterator it3 = it2.data().begin();
        for ( ; it3 != it2.data().end(); ++it3 ) {
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
        kdDebug(5300) << "\n\nLdapSearch::makeSearchData() found a list: " << name << "\n\n" << endl;
        ret.append( name );
        // following lines commented out for bugfixing kolab issue #177:
        //
        // Unlike we thought previously we may NOT append the server name here.
        //
        // The right server is found by the SMTP server instead: Kolab users
        // must use the correct SMTP server, by definition.
        //
        //mail = (*it1).client->base().simplifyWhiteSpace();
        //mail.replace( ",dc=", ".", false );
        //if( mail.startsWith("dc=", false) )
        //  mail.remove(0, 3);
        //mail.prepend( '@' );
        //mail.prepend( name );
        //mail = name;
      } else {
        kdDebug(5300) << "LdapSearch::makeSearchData() found BAD ENTRY: \"" << name << "\"" << endl;
        continue; // nothing, bad entry
      }
    } else if ( name.isEmpty() ) {
      kdDebug(5300) << "LdapSearch::makeSearchData() mail: \"" << mail << "\"" << endl;
      ret.append( mail );
    } else {
      kdDebug(5300) << "LdapSearch::makeSearchData() name: \"" << name << "\"  mail: \"" << mail << "\"" << endl;
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
