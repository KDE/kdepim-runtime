/* kldapclient.h - LDAP access
 *      Copyright (C) 2002 Klarälvdalens Datakonsult AB
 *
 *      Author: Steffen Hansen <hansen@kde.org>
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


#ifndef KPIM_LDAPCLIENT_H
#define KPIM_LDAPCLIENT_H


#include <qobject.h>
#include <qstring.h>
#include <qcstring.h>
#include <qstringlist.h>
#include <qmemarray.h>
#include <qguardedptr.h>
#include <qtimer.h>

#include <kio/job.h>
#include <kabc/ldif.h>

#include <kdepimmacros.h>

namespace KPIM {

class LdapClient;
typedef QValueList<QByteArray> LdapAttrValue;
typedef QMap<QString,LdapAttrValue > LdapAttrMap;

class LdapServer
{
  public:
    LdapServer() : mPort( 389 ) {}

    enum Security{ Sec_None, TLS, SSL };
    enum Auth{ Anonymous, Simple, SASL };
    QString host() const { return mHost; }
    int port() const { return mPort; }
    const QString &baseDN() const { return mBaseDN; }
    const QString &user() const { return mUser; }
    const QString &bindDN() const { return mBindDN; }
    const QString &pwdBindDN() const { return mPwdBindDN; }
    int timeLimit() const { return mTimeLimit; }
    int sizeLimit() const { return mSizeLimit; }
    int version() const { return mVersion; }
    int security() const { return mSecurity; }
    int auth() const { return mAuth; }
    const QString &mech() const { return mMech; }

    void setHost( const QString &host ) { mHost = host; }
    void setPort( int port ) { mPort = port; }
    void setBaseDN( const QString &baseDN ) {  mBaseDN = baseDN; }
    void setUser( const QString &user ) { mUser = user; }
    void setBindDN( const QString &bindDN ) {  mBindDN = bindDN; }
    void setPwdBindDN( const QString &pwdBindDN ) {  mPwdBindDN = pwdBindDN; }
    void setTimeLimit( int timelimit ) { mTimeLimit = timelimit; }
    void setSizeLimit( int sizelimit ) { mSizeLimit = sizelimit; }
    void setVersion( int version ) { mVersion = version; }
    void setSecurity( int security ) { mSecurity = security; } //0-No, 1-TLS, 2-SSL - KDE4: add an enum to Lda
    void setAuth( int auth ) { mAuth = auth; }; //0-Anonymous, 1-simple, 2-SASL - KDE4: add an enum to LdapCon
    void setMech( const QString &mech ) { mMech = mech; }

  private:
    QString mHost;
    int mPort;
    QString mBaseDN;
    QString mUser;
    QString mBindDN;
    QString mPwdBindDN;
    QString mMech;
    int mTimeLimit, mSizeLimit, mVersion, mSecurity, mAuth;
};


/**
  * This class is internal. Binary compatibiliy might be broken any time
  * without notification. Do not use it.
  *
  * We mean it!
  *
  */
class LdapObject
{
  public:
    LdapObject()
      : dn( QString::null ), client( 0 ) {}
    explicit LdapObject( const QString& _dn, LdapClient* _cl ) : dn( _dn ), client( _cl ) {}
    LdapObject( const LdapObject& that ) { assign( that ); }

    LdapObject& operator=( const LdapObject& that )
    {
      assign( that );
      return *this;
    }

    QString toString() const;

    void clear();

    QString dn;
    QString objectClass;
    LdapAttrMap attrs;
    LdapClient* client;

  protected:
    void assign( const LdapObject& that );

  private:
    //class LdapObjectPrivate* d;
};

/**
  * This class is internal. Binary compatibility might be broken any time
  * without notification. Do not use it.
  *
  * We mean it!
  *
  */
class KDE_EXPORT LdapClient : public QObject
{
  Q_OBJECT

  public:
    LdapClient( int clientNumber, QObject* parent = 0, const char* name = 0 );
    virtual ~LdapClient();

    /*! returns true if there is a query running */
    bool isActive() const { return mActive; }

    int clientNumber() const;
    int completionWeight() const;
    void setCompletionWeight( int );

    const LdapServer& server() { return mServer; }
    void setServer( const LdapServer &server ) { mServer = server; }
    /*! Return the attributes that should be
     * returned, or an empty list if
     * all attributes are wanted
     */
    QStringList attrs() const { return mAttrs; }

  signals:
    /*! Emitted when the query is done */
    void done();

    /*! Emitted in case of error */
    void error( const QString& );

    /*! Emitted once for each object returned
     * from the query
     */
    void result( const KPIM::LdapObject& );

  public slots: // why are those slots?
    /*! Set the attributes that should be
     * returned, or an empty list if
     * all attributes are wanted
     */
    void setAttrs( const QStringList& attrs );

    void setScope( const QString scope ) { mScope = scope; }

    /*!
     * Start the query with filter filter
     */
    void startQuery( const QString& filter );

    /*!
     * Abort a running query
     */
    void cancelQuery();

  protected slots:
    void slotData( KIO::Job*, const QByteArray &data );
    void slotInfoMessage( KIO::Job*, const QString &info );
    void slotDone();

  protected:
    void startParseLDIF();
    void parseLDIF( const QByteArray& data );
    void endParseLDIF();
    void finishCurrentObject();

    LdapServer mServer;
    QString mScope;
    QStringList mAttrs;

    QGuardedPtr<KIO::SimpleJob> mJob;
    bool mActive;
    bool mReportObjectClass;

    LdapObject mCurrentObject;

  private:
    KABC::LDIF mLdif;
    int mClientNumber;
    int mCompletionWeight;

//    class LdapClientPrivate;
//    LdapClientPrivate* d;
};

/**
 * Structure describing one result returned by a LDAP query
 */
struct LdapResult {
  QString name;     ///< full name
  QStringList email;    ///< emails
  int clientNumber; ///< for sorting in a ldap-only lookup
  int completionWeight; ///< for sorting in a completion list
};
typedef QValueList<LdapResult> LdapResultList;


/**
  * This class is internal. Binary compatibiliy might be broken any time
  * without notification. Do not use it.
  *
  * We mean it!
  *
  */
class KDE_EXPORT LdapSearch : public QObject
{
  Q_OBJECT

  public:
    LdapSearch();

    static KConfig *config();
    static void readConfig( LdapServer &server, KConfig *config, int num, bool active );
    static void writeConfig( const LdapServer &server, KConfig *config, int j, bool active );

    void startSearch( const QString& txt );
    void cancelSearch();
    bool isAvailable() const;

    QValueList< LdapClient* > clients() const { return mClients; }

  signals:
    /// Results, assembled as "Full Name <email>"
    /// (This signal can be emitted many times)
    void searchData( const QStringList& );
    /// Another form for the results, with separate fields
    /// (This signal can be emitted many times)
    void searchData( const KPIM::LdapResultList& );
    void searchDone();

  private slots:
    void slotLDAPResult( const KPIM::LdapObject& );
    void slotLDAPError( const QString& );
    void slotLDAPDone();
    void slotDataTimer();
    void slotFileChanged( const QString& );

  private:
    void readConfig();
    void finish();
    void makeSearchData( QStringList& ret, LdapResultList& resList );
    QValueList< LdapClient* > mClients;
    QString mSearchText;
    QTimer mDataTimer;
    int mActiveClients;
    bool mNoLDAPLookup;
    QValueList< LdapObject > mResults;
    QString mConfigFile;

  private:
    static KConfig *s_config;
    class LdapSearchPrivate* d;
};

}
#endif // KPIM_LDAPCLIENT_H
