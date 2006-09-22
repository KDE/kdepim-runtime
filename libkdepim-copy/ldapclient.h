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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */


#ifndef KPIM_LDAPCLIENT_H
#define KPIM_LDAPCLIENT_H


#include <QObject>
#include <QString>
#include <q3cstring.h>
#include <QStringList>
#include <q3memarray.h>
#include <QPointer>
#include <QTimer>

#include <kio/job.h>
#include <kldap/ldif.h>
#include <kldap/ldapobject.h>
#include <kldap/ldapserver.h>

#include <kdepimmacros.h>

class KConfig;

namespace KPIM {

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

    const KLDAP::LdapServer& server() { return mServer; }
    void setServer( const KLDAP::LdapServer &server ) { mServer = server; }
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
    void result( const LdapClient &client, const KLDAP::LdapObject& );

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
    void slotInfoMessage( KJob*, const QString &info, const QString& );
    void slotDone();

  protected:
    void startParseLDIF();
    void parseLDIF( const QByteArray& data );
    void endParseLDIF();
    void finishCurrentObject();

    KLDAP::LdapServer mServer;
    QString mScope;
    QStringList mAttrs;

    QPointer<KIO::SimpleJob> mJob;
    bool mActive;

    KLDAP::LdapObject mCurrentObject;

  private:
    KLDAP::Ldif mLdif;
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
typedef QList<LdapResult> LdapResultList;


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
    static void readConfig( KLDAP::LdapServer &server, KConfig *config, int num, bool active );
    static void writeConfig( const KLDAP::LdapServer &server, KConfig *config, int j, bool active );

    void startSearch( const QString& txt );
    void cancelSearch();
    bool isAvailable() const;

    QList< LdapClient* > clients() const { return mClients; }

  signals:
    /// Results, assembled as "Full Name <email>"
    /// (This signal can be emitted many times)
    void searchData( const QStringList& );
    /// Another form for the results, with separate fields
    /// (This signal can be emitted many times)
    void searchData( const KPIM::LdapResultList& );
    void searchDone();

  private slots:
    void slotLDAPResult( const LdapClient& client, const KLDAP::LdapObject& );
    void slotLDAPError( const QString& );
    void slotLDAPDone();
    void slotDataTimer();
    void slotFileChanged( const QString& );

  private:
    
    typedef struct ResultObject {
      const LdapClient *client;
      KLDAP::LdapObject object;
    };
    
    void readConfig();
    void finish();
    void makeSearchData( QStringList& ret, LdapResultList& resList );
    QList< LdapClient* > mClients;
    QString mSearchText;
    QTimer mDataTimer;
    int mActiveClients;
    bool mNoLDAPLookup;
    QList< ResultObject > mResults;
    QString mConfigFile;

  private:
    static KConfig *s_config;
    class LdapSearchPrivate* d;
};

}
#endif // KPIM_LDAPCLIENT_H
