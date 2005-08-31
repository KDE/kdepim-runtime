/* Simple Addressbook for KMail
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef KAddrBook_h
#define KAddrBook_h

#include <qstringlist.h>

#include <kdeversion.h>
#include <kabc/addressee.h>
#include <kdepimmacros.h>

class QWidget;

class KDE_EXPORT KAddrBookExternal {
public:
  static void addEmail( const QString &addr, QWidget *parent );
  static void addNewAddressee( QWidget* );
  static void openEmail( const QString &email, const QString &addr, QWidget *parent );
  static void openAddressBook( QWidget *parent );

  static bool addVCard( const KABC::Addressee& addressee, QWidget *parent );

  static QString expandDistributionList( const QString& listName );
private:
  static bool addAddressee( const KABC::Addressee& addressee );
};

#endif /*KAddrBook_h*/
