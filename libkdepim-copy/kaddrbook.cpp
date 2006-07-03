// -*- mode: C++; c-file-style: "gnu" -*-
// kaddrbook.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include <config.h>

#include "kaddrbook.h"

#ifdef KDEPIM_NEW_DISTRLISTS
#include "distributionlist.h"
#else
#include <kabc/distributionlist.h>
#endif

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdeversion.h>
#include <kabc/resource.h>
#include <kabc/stdaddressbook.h>
#include <kabc/vcardconverter.h>
#include <kresources/selectdialog.h>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <QEventLoop>
#include <QRegExp>
#include <QList>

#include <unistd.h>
#include <ktoolinvocation.h>

//-----------------------------------------------------------------------------
void KAddrBookExternal::openEmail( const QString &email, const QString &addr, QWidget *) {
  //QString email = KMMessage::getEmailAddr(addr);
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  KABC::Addressee::List addresseeList = addressBook->findByEmail(email);
  QDBusInterface abinterface( "org.kde.pim.kaddressbook", "/", "org.kde.pim.Addressbook" );
  if ( abinterface.isValid() ) {
    //make sure kaddressbook is loaded, otherwise showContactEditor
    //won't work as desired, see bug #87233
    abinterface.call( "newInstance" );
  }
  else
    KToolInvocation::startServiceByDesktopName( "kaddressbook" );

  QDBusInterface abinterface1( "org.kde.pim.kaddressbook", "/", "org.kde.pim.Addressbook" );
  if( !addresseeList.isEmpty() ) {
    abinterface1.call( "showContactEditor", addresseeList.first().uid() );
  }
  else {
    abinterface1.call( "addEmail", addr );
  }
}

//-----------------------------------------------------------------------------
void KAddrBookExternal::addEmail( const QString& addr, QWidget *parent) {
  QString email;
  QString name;

  KABC::Addressee::parseEmailAddress( addr, name, email );

  KABC::AddressBook *ab = KABC::StdAddressBook::self( true );

  // force a reload of the address book file so that changes that were made
  // by other programs are loaded
  ab->asyncLoad();

  KABC::Addressee::List addressees = ab->findByEmail( email );

  if ( addressees.isEmpty() ) {
    KABC::Addressee a;
    a.setNameFromString( name );
    a.insertEmail( email, true );

    if ( !KAddrBookExternal::addAddressee( a ) ) {
      KMessageBox::error( parent, i18n("Cannot save to addressbook.") );
    } else {
      QString text = i18n("<qt>The email address <b>%1</b> was added to your "
                          "addressbook; you can add more information to this "
                          "entry by opening the addressbook.</qt>", addr );
      KMessageBox::information( parent, text, QString(), "addedtokabc" );
    }
  } else {
    QString text = i18n("<qt>The email address <b>%1</b> is already in your "
                        "addressbook.</qt>", addr );
    KMessageBox::information( parent, text, QString(),
                              "alreadyInAddressBook" );
  }
}

void KAddrBookExternal::openAddressBook(QWidget *) {
  KToolInvocation::startServiceByDesktopName( "kaddressbook" );
}

void KAddrBookExternal::addNewAddressee( QWidget* )
{
  KToolInvocation::startServiceByDesktopName("kaddressbook");
  QDBusInterface call("org.kde.pim.kaddressboook", "/", "org.kde.pim.Addressbook" );
  call.call("newContact");
}

bool KAddrBookExternal::addVCard( const KABC::Addressee& addressee, QWidget *parent )
{
  KABC::AddressBook *ab = KABC::StdAddressBook::self( true );
  bool inserted = false;

  KABC::Addressee::List addressees =
      ab->findByEmail( addressee.preferredEmail() );

  if ( addressees.isEmpty() ) {
    if ( !KAddrBookExternal::addAddressee( addressee ) ) {
      KMessageBox::error( parent, i18n("Cannot save to addressbook.") );
      inserted = false;
    } else {
      QString text = i18n("The VCard was added to your addressbook; "
                          "you can add more information to this "
                          "entry by opening the addressbook.");
      KMessageBox::information( parent, text, QString(), "addedtokabc" );
      inserted = true;
    }
  } else {
    QString text = i18n("The VCard's primary email address is already in "
                        "your addressbook; however, you may save the VCard "
                        "into a file and import it into the addressbook "
                        "manually.");
    KMessageBox::information( parent, text );
    inserted = true;
  }

  return inserted;
}

bool KAddrBookExternal::addAddressee( const KABC::Addressee& addr )
{
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );

  // PORT. FIXME: This ugly hack will be removed in 4.0
  while ( !addressBook->loadingHasFinished() ) {
    qApp->processEvents( QEventLoop::ExcludeUserInput );

    // use sleep here to reduce cpu usage
    usleep( 100 );
  }

  // Select a resource
  QList<KABC::Resource*> kabcResources = addressBook->resources();
  QList<KRES::Resource*> kresResources;
  QListIterator<KABC::Resource*> resIt( kabcResources );
  KABC::Resource *kabcResource;
  while ( resIt.hasNext() ) {
    kabcResource = resIt.next();
    if ( !kabcResource->readOnly() ) {
      KRES::Resource *res = static_cast<KRES::Resource*>( kabcResource );
      if ( res )
        kresResources.append( res );
    }
  }

  kabcResource = static_cast<KABC::Resource*>( KRES::SelectDialog::getResource( kresResources, 0 ) );

  KABC::Ticket *ticket = addressBook->requestSaveTicket( kabcResource );
  bool saved = false;
  if ( ticket ) {
    KABC::Addressee addressee( addr );
    addressee.setResource( kabcResource );
    addressBook->insertAddressee( addressee );
    saved = addressBook->save( ticket );
    if ( !saved )
      addressBook->releaseSaveTicket( ticket );
  }

  addressBook->emitAddressBookChanged();

  return saved;
}

QString KAddrBookExternal::expandDistributionList( const QString& listName )
{
  if ( listName.isEmpty() )
    return QString();

  const QString lowerListName = listName.toLower();
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
#ifdef KDEPIM_NEW_DISTRLISTS
  KPIM::DistributionList distrList = KPIM::DistributionList::findByName( addressBook, lowerListName, false );
  if ( !distrList.isEmpty() ) {
    return distrList.emails( addressBook ).join( ", " );
  }
#else
  KABC::DistributionListManager manager( addressBook );
  manager.load();
  const QStringList listNames = manager.listNames();

  for ( QStringList::ConstIterator it = listNames.begin();
        it != listNames.end(); ++it) {
    if ( (*it).toLower() == lowerListName ) {
      const QStringList addressList = manager.list( *it )->emails();
      return addressList.join( ", " );
    }
  }
#endif
  return QString();
}
