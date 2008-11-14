/*
  Simple Addressbook for KMail
  Copyright Stefan Taferner <taferner@kde.org>

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
*/

#include "kaddrbookexternal.h"
#include "kaddressbookcore_interface.h"

#include <kabc/distributionlist.h>
#include <kabc/resource.h>
#include <kabc/stdaddressbook.h>
#include <kabc/vcardconverter.h>
#include <kabc/errorhandler.h>
#include <kresources/selectdialog.h>

#include <kdefakes.h> // usleep
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KToolInvocation>

#include <QApplication>
#include <QEventLoop>
#include <QList>
#include <QRegExp>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <unistd.h>

using namespace KPIM;

//-----------------------------------------------------------------------------
void KAddrBookExternal::openEmail( const QString &email, const QString &addr, QWidget *) {
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  KABC::Addressee::List addresseeList = addressBook->findByEmail(email);

  // If KAddressbook is running, talk to it, otherwise start it.
  QDBusInterface abinterface( "org.kde.kaddressbook", "/kaddressbook_PimApplication",
                              "org.kde.KUniqueApplication" );

  if ( abinterface.isValid() ) {
    //make sure kaddressbook is loaded, otherwise showContactEditor
    //won't work as desired, see bug #87233
    abinterface.call("newInstance", QByteArray(), QByteArray());
  } else {
    KToolInvocation::startServiceByDesktopName( "kaddressbook" );
  }

  OrgKdeKAddressbookCoreInterface interface( "org.kde.kaddressbook",
                                             "/KAddressBook",
                                             QDBusConnection::sessionBus() );
  if( !addresseeList.isEmpty() ) {
    interface.showContactEditor( addresseeList.first().uid() );
  } else {
    interface.addEmail( addr );
  }
}

//-----------------------------------------------------------------------------
void KAddrBookExternal::addEmail( const QString &addr, QWidget *parent )
{
  QString email;
  QString name;

  KABC::Addressee::parseEmailAddress( addr, name, email );
  KABC::AddressBook *ab = KABC::StdAddressBook::self( true );
  ab->setErrorHandler( new KABC::GuiErrorHandler( parent ) );

  // force a reload of the address book file so that changes that were made
  // by other programs are loaded
  ab->asyncLoad();

  KABC::Addressee::List addressees = ab->findByEmail( email );

  if ( addressees.isEmpty() ) {
    KABC::Addressee a;
    a.setNameFromString( name );
    a.insertEmail( email, true );

    if ( KAddrBookExternal::addAddressee( a ) ) {
      QString text = i18n( "<qt>The email address <b>%1</b> was added to your "
                           "addressbook; you can add more information to this "
                           "entry by opening the addressbook.</qt>", addr );
      KMessageBox::information( parent, text, QString(), "addedtokabc" );
    }
  } else {
    QString text =
      i18n( "<qt>The email address <b>%1</b> is already in your addressbook.</qt>", addr );
    KMessageBox::information( parent, text, QString(), "alreadyInAddressBook" );
  }
  ab->setErrorHandler( 0 );
}

void KAddrBookExternal::openAddressBook( QWidget * )
{
  KToolInvocation::startServiceByDesktopName( "kaddressbook" );
}

void KAddrBookExternal::addNewAddressee( QWidget * )
{
  KToolInvocation::startServiceByDesktopName( "kaddressbook" );
  OrgKdeKAddressbookCoreInterface interface( "org.kde.kaddressbook",
                                             "/KAddressBook",
                                             QDBusConnection::sessionBus() );
  interface.newContact();
}

bool KAddrBookExternal::addVCard( const KABC::Addressee &addressee, QWidget *parent )
{
  KABC::AddressBook *ab = KABC::StdAddressBook::self( true );
  bool inserted = false;

  ab->setErrorHandler( new KABC::GuiErrorHandler( parent ) );

  KABC::Addressee::List addressees =
    ab->findByEmail( addressee.preferredEmail() );

  if ( addressees.isEmpty() ) {
    if ( KAddrBookExternal::addAddressee( addressee ) ) {
      QString text = i18n( "The VCard was added to your addressbook; "
                           "you can add more information to this "
                           "entry by opening the addressbook." );
      KMessageBox::information( parent, text, QString(), "addedtokabc" );
      inserted = true;
    }
  } else {
    QString text = i18n( "The VCard's primary email address is already in "
                         "your addressbook; however, you may save the VCard "
                         "into a file and import it into the addressbook manually." );
    KMessageBox::information( parent, text );
    inserted = true;
  }

  ab->setErrorHandler( 0 );
  return inserted;
}

bool KAddrBookExternal::addAddressee( const KABC::Addressee &addr )
{
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );

  // PORT. FIXME: This ugly hack will be removed in 4.0
  while ( !addressBook->loadingHasFinished() ) {
    qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

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
      if ( res ) {
        kresResources.append( res );
      }
    }
  }

  kabcResource =
    static_cast<KABC::Resource*>( KRES::SelectDialog::getResource( kresResources, 0 ) );

  KABC::Ticket *ticket = addressBook->requestSaveTicket( kabcResource );
  bool saved = false;
  if ( ticket ) {
    KABC::Addressee addressee( addr );
    addressee.setResource( kabcResource );
    addressBook->insertAddressee( addressee );
    saved = addressBook->save( ticket );
    if ( !saved ) {
      addressBook->releaseSaveTicket( ticket );
    }
  }

  addressBook->emitAddressBookChanged();

  return saved;
}

QString KAddrBookExternal::expandDistributionList( const QString &listName )
{
  if ( listName.isEmpty() ) {
    return QString();
  }

  const QString lowerListName = listName.toLower();
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  KABC::DistributionList* list = addressBook->findDistributionListByName( listName, Qt::CaseInsensitive );

  if ( list ) {
    return list->emails().join( ", " );
  }
  return QString();
}
