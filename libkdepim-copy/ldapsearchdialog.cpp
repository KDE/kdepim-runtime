/* ldapsearchdialogimpl.cpp - LDAP access
 *      Copyright (C) 2002 Klar�vdalens Datakonsult AB
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

#include "ldapsearchdialog.h"
#include "ldapclient.h"

#include <qcheckbox.h>
#include <qevent.h>
#include <qgroupbox.h>
#include <q3header.h>
#include <qlabel.h>
#include <qlayout.h>
#include <q3listview.h>
#include <qlistview.h>
#include <qpushbutton.h>

#include <kabc/addresslineedit.h>
#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktoolinvocation.h>

using namespace KPIM;

static QString asUtf8( const QByteArray &val )
{
  if ( val.isEmpty() )
    return QString::null;

  const char *data = val.data();

  //QString::fromUtf8() bug workaround
  if ( data[ val.size() - 1 ] == '\0' )
    return QString::fromUtf8( data, val.size() - 1 );
  else
    return QString::fromUtf8( data, val.size() );
}

static QString join( const KLDAP::LdapAttrValue& lst, const QString& sep )
{
  QString res;
  bool alredy = false;
  for ( KLDAP::LdapAttrValue::ConstIterator it = lst.begin(); it != lst.end(); ++it ) {
    if ( alredy )
      res += sep;
    alredy = TRUE;
    res += asUtf8( *it );
  }
  return res;
}

static QMap<QString, QString>& adrbookattr2ldap()
{
  static QMap<QString, QString> keys;

  if ( keys.isEmpty() ) {
    keys[ i18n( "Title" ) ] = "title";
    keys[ i18n( "Full Name" ) ] = "cn";
    keys[ i18n( "Email" ) ] = "mail";
    keys[ i18n( "Home Number" ) ] = "homePhone";
    keys[ i18n( "Work Number" ) ] = "telephoneNumber";
    keys[ i18n( "Mobile Number" ) ] = "mobile";
    keys[ i18n( "Fax Number" ) ] = "facsimileTelephoneNumber";
    keys[ i18n( "Pager" ) ] = "pager";
    keys[ i18n( "Street") ] = "street";
    keys[ i18n( "State" ) ] = "st";
    keys[ i18n( "Country" ) ] = "co";
    keys[ i18n( "City" ) ] = "l";
    keys[ i18n( "Organization" ) ] = "o";
    keys[ i18n( "Company" ) ] = "Company";
    keys[ i18n( "Department" ) ] = "department";
    keys[ i18n( "Zip Code" ) ] = "postalCode";
    keys[ i18n( "Postal Address" ) ] = "postalAddress";
    keys[ i18n( "Description" ) ] = "description";
    keys[ i18n( "User ID" ) ] = "uid";
  }
  return keys;
}

class ContactListItem : public Q3ListViewItem
{
  public:
    ContactListItem( Q3ListView* parent, const KLDAP::LdapAttrMap& attrs )
      : Q3ListViewItem( parent ), mAttrs( attrs )
    { }

    KLDAP::LdapAttrMap mAttrs;

    virtual QString text( int col ) const
    {
      // Look up a suitable attribute for column col
      const QString colName = listView()->columnText( col );
      const QString ldapAttrName = adrbookattr2ldap()[ colName ];
      return join( mAttrs[ ldapAttrName ], ", " );
    }
};

LdapSearchDialog::LdapSearchDialog( QWidget* parent, const char* name )
  : KDialog( parent )
{
  setObjectName( name );
  setCaption( i18n( "Search for Addresses in Directory" ) );
  setButtons( Help | User1 | User2 | User3 | Cancel );
  setDefaultButton( Default );
  showButtonSeparator( true );

  setButtonGuiItem( Cancel, KStandardGuiItem::close() );
  QFrame *page = new QFrame( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout();
  topLayout->setContentsMargins( marginHint(), marginHint(),
                                 marginHint(), marginHint() );
  topLayout->setSpacing( spacingHint() );
  page->setLayout( topLayout );

  QGroupBox *groupBox = new QGroupBox( i18n( "Search for Addresses in Directory" ),
                                       page );
  QGridLayout *boxLayout = new QGridLayout();
  boxLayout->setSpacing( spacingHint() );
  groupBox->setLayout( boxLayout );
  boxLayout->setColumnStretch( 1, 1 );

  QLabel *label = new QLabel( i18n( "Search for:" ), groupBox );
  boxLayout->addWidget( label, 0, 0 );

  mSearchEdit = new KLineEdit( groupBox );
  boxLayout->addWidget( mSearchEdit, 0, 1 );
  label->setBuddy( mSearchEdit );

  label = new QLabel( i18n( "in" ), groupBox );
  boxLayout->addWidget( label, 0, 2 );

  mFilterCombo = new KComboBox( groupBox );
  mFilterCombo->addItem( i18n( "Name" ) );
  mFilterCombo->addItem( i18n( "Email" ) );
  mFilterCombo->addItem( i18n( "Home Number" ) );
  mFilterCombo->addItem( i18n( "Work Number" ) );
  boxLayout->addWidget( mFilterCombo, 0, 3 );

  QSize buttonSize;
  mSearchButton = new QPushButton( i18n( "Stop" ), groupBox );
  buttonSize = mSearchButton->sizeHint();
  mSearchButton->setText( i18n( "Search" ) );
  if ( buttonSize.width() < mSearchButton->sizeHint().width() )
    buttonSize = mSearchButton->sizeHint();
  mSearchButton->setFixedWidth( buttonSize.width() );

  mSearchButton->setDefault( true );
  boxLayout->addWidget( mSearchButton, 0, 4 );

  mRecursiveCheckbox = new QCheckBox( i18n( "Recursive search" ), groupBox  );
  mRecursiveCheckbox->setChecked( true );
  boxLayout->addWidget( mRecursiveCheckbox, 1, 0, 1, 4 );

  mSearchType = new KComboBox( groupBox );
  mSearchType->addItem( i18n( "Contains" ) );
  mSearchType->addItem( i18n( "Starts With" ) );
  boxLayout->addWidget( mSearchType, 1, 3, 1, 2 );

  topLayout->addWidget( groupBox );

  mResultListView = new Q3ListView( page );
  mResultListView->setSelectionMode( Q3ListView::Multi );
  mResultListView->setAllColumnsShowFocus( true );
  mResultListView->setShowSortIndicator( true );
  topLayout->addWidget( mResultListView );

  resize( QSize( 600, 400).expandedTo( minimumSizeHint() ) );

  setButtonText( User1, i18n( "Unselect All" ) );
  setButtonText( User2, i18n( "Select All" ) );
  setButtonText( User3, i18n( "Add Selected" ) );

  mNumHosts = 0;
  mIsOK = false;

  connect( mRecursiveCheckbox, SIGNAL( toggled( bool ) ),
	   this, SLOT( slotSetScope( bool ) ) );
  connect( mSearchButton, SIGNAL( clicked() ),
	   this, SLOT( slotStartSearch() ) );

  setTabOrder(mSearchEdit, mFilterCombo);
  setTabOrder(mFilterCombo, mSearchButton);
  mSearchEdit->setFocus();

  restoreSettings();
}

LdapSearchDialog::~LdapSearchDialog()
{
  saveSettings();
}

void LdapSearchDialog::restoreSettings()
{
  // Create one KPIM::LdapClient per selected server and configure it.

  // First clean the list to make sure it is empty at
  // the beginning of the process
  qDeleteAll( mLdapClientList );
  mLdapClientList.clear();

  KConfig kabConfig( "kaddressbookrc" );
  mSearchType->setCurrentIndex( kabConfig.group("LDAPSearch").readEntry( "SearchType", 0 ) );

  // then read the config file and register all selected
  // server in the list
  KConfig config( "kabldaprc" );
  KConfigGroup saver = config.group( "LDAP" );
  mNumHosts = saver.readEntry( "NumSelectedHosts", 0 );
  if ( !mNumHosts ) {
    KMessageBox::error( this, i18n( "You must select a LDAP server before searching.\nYou can do this from the menu Settings/Configure KAddressBook." ) );
    mIsOK = false;
  } else {
    mIsOK = true;
    for ( int j = 0; j < mNumHosts; ++j ) {
      KLDAP::LdapServer ldapServer;

      QString host = saver.readEntry( QString( "SelectedHost%1" ).arg( j ), "" );
      if ( !host.isEmpty() )
        ldapServer.setHost( host );

      int port = saver.readEntry( QString( "SelectedPort%1" ).arg( j ), 0 );
      if ( port )
        ldapServer.setPort( port );

      QString base = saver.readEntry( QString( "SelectedBase%1" ).arg( j ), "" );
      if ( !base.isEmpty() )
        ldapServer.setBaseDn( KLDAP::LdapDN( base ) );

      QString bindDN = saver.readEntry( QString( "SelectedBind%1" ).arg( j ), "" );
      if ( !bindDN.isEmpty() )
        ldapServer.setBindDn( bindDN );

      QString pwdBindDN = saver.readEntry( QString( "SelectedPwdBind%1" ).arg( j ), "" );
      if ( !pwdBindDN.isEmpty() )
        ldapServer.setPassword( pwdBindDN );

      KPIM::LdapClient* ldapClient = new KPIM::LdapClient( 0, this, "ldapclient" );
      ldapClient->setServer( ldapServer );

      QStringList attrs;

      for ( QMap<QString,QString>::Iterator it = adrbookattr2ldap().begin(); it != adrbookattr2ldap().end(); ++it )
        attrs << *it;

      ldapClient->setAttrs( attrs );

      connect( ldapClient, SIGNAL( result( const KPIM::LdapClient&, const KLDAP::LdapObject& ) ),
	       this, SLOT( slotAddResult( const KPIM::LdapClient&, const KLDAP::LdapObject& ) ) );
      connect( ldapClient, SIGNAL( done() ),
	       this, SLOT( slotSearchDone() ) );
      connect( ldapClient, SIGNAL( error( const QString& ) ),
	       this, SLOT( slotError( const QString& ) ) );

      mLdapClientList.append( ldapClient );
    }

/** CHECKIT*/
    while ( mResultListView->header()->count() > 0 ) {
      mResultListView->removeColumn(0);
    }

    mResultListView->addColumn( i18n( "Full Name" ) );
    mResultListView->addColumn( i18n( "Email" ) );
    mResultListView->addColumn( i18n( "Home Number" ) );
    mResultListView->addColumn( i18n( "Work Number" ) );
    mResultListView->addColumn( i18n( "Mobile Number" ) );
    mResultListView->addColumn( i18n( "Fax Number" ) );
    mResultListView->addColumn( i18n( "Company" ) );
    mResultListView->addColumn( i18n( "Organization" ) );
    mResultListView->addColumn( i18n( "Street" ) );
    mResultListView->addColumn( i18n( "State" ) );
    mResultListView->addColumn( i18n( "Country" ) );
    mResultListView->addColumn( i18n( "Zip Code" ) );
    mResultListView->addColumn( i18n( "Postal Address" ) );
    mResultListView->addColumn( i18n( "City" ) );
    mResultListView->addColumn( i18n( "Department" ) );
    mResultListView->addColumn( i18n( "Description" ) );
    mResultListView->addColumn( i18n( "User ID" ) );
    mResultListView->addColumn( i18n( "Title" ) );

    mResultListView->clear();
  }
}

void LdapSearchDialog::saveSettings()
{
  KConfig config( "kaddressbookrc" );
  KConfigGroup group = config.group("LDAPSearch");
  group.writeEntry( "SearchType", mSearchType->currentIndex() );
  config.sync();
}

void LdapSearchDialog::cancelQuery()
{
  foreach ( KPIM::LdapClient* client, mLdapClientList ) {
    client->cancelQuery();
  }
}

void LdapSearchDialog::slotAddResult( const KPIM::LdapClient &, const KLDAP::LdapObject& obj )
{
  new ContactListItem( mResultListView, obj.attributes() );
}

void LdapSearchDialog::slotSetScope( bool rec )
{
  foreach ( KPIM::LdapClient* client, mLdapClientList ) {
    if ( rec )
      client->setScope( "sub" );
    else
      client->setScope( "one" );
  }
}

QString LdapSearchDialog::makeFilter( const QString& query, const QString& attr,
                                      bool startsWith )
{
  /* The reasoning behind this filter is:
   * If it's a person, or a distlist, show it, even if it doesn't have an email address.
   * If it's not a person, or a distlist, only show it if it has an email attribute.
   * This allows both resource accounts with an email address which are not a person and
   * person entries without an email address to show up, while still not showing things
   * like structural entries in the ldap tree. */
  QString result( "&(|(objectclass=person)(objectclass=groupofnames)(mail=*))(" );
  if( query.isEmpty() )
    // Return a filter that matches everything
    return result + "|(cn=*)(sn=*)" + ")";

  if ( attr == i18n( "Name" ) ) {
    result += startsWith ? "|(cn=%1*)(sn=%2*)" : "|(cn=*%1*)(sn=*%2*)";
    result = result.arg( query ).arg( query );
  } else {
    result += (startsWith ? "%1=%2*" : "%1=*%2*");
    if ( attr == i18n( "Email" ) ) {
      result = result.arg( "mail" ).arg( query );
    } else if ( attr == i18n( "Home Number" ) ) {
      result = result.arg( "homePhone" ).arg( query );
    } else if ( attr == i18n( "Work Number" ) ) {
      result = result.arg( "telephoneNumber" ).arg( query );
    } else {
      // Error?
      result = QString::null;
      return result;
    }
  }
  result += ")";
  return result;
}

void LdapSearchDialog::slotStartSearch()
{
  cancelQuery();

  QApplication::setOverrideCursor( Qt::waitCursor );
  mSearchButton->setText( i18n( "Stop" ) );

  disconnect( mSearchButton, SIGNAL( clicked() ),
              this, SLOT( slotStartSearch() ) );
  connect( mSearchButton, SIGNAL( clicked() ),
           this, SLOT( slotStopSearch() ) );

  bool startsWith = (mSearchType->currentIndex() == 1);

  QString filter = makeFilter( mSearchEdit->text().trimmed(), mFilterCombo->currentText(), startsWith );

   // loop in the list and run the KPIM::LdapClients
  mResultListView->clear();
  foreach( KPIM::LdapClient* client, mLdapClientList ) {
    client->startQuery( filter );
  }

  saveSettings();
}

void LdapSearchDialog::slotStopSearch()
{
  cancelQuery();
  slotSearchDone();
}

void LdapSearchDialog::slotSearchDone()
{
  // If there are no more active clients, we are done.
  foreach ( KPIM::LdapClient* client,  mLdapClientList ) {
    if ( client->isActive() )
      return;
  }

  disconnect( mSearchButton, SIGNAL( clicked() ),
              this, SLOT( slotStopSearch() ) );
  connect( mSearchButton, SIGNAL( clicked() ),
           this, SLOT( slotStartSearch() ) );

  mSearchButton->setText( i18n( "Search" ) );
  QApplication::restoreOverrideCursor();
}

void LdapSearchDialog::slotError( const QString& error )
{
  QApplication::restoreOverrideCursor();
  KMessageBox::error( this, error );
}

void LdapSearchDialog::closeEvent( QCloseEvent* e )
{
  slotStopSearch();
  e->accept();
}

/*!
 * Returns a ", " separated list of email addresses that were
 * checked by the user
 */
QString LdapSearchDialog::selectedEMails() const
{
  QStringList result;
  ContactListItem* cli = static_cast<ContactListItem*>( mResultListView->firstChild() );
  while ( cli ) {
    if ( cli->isSelected() ) {
      QString email = asUtf8( cli->mAttrs[ "mail" ].first() ).trimmed();
      if ( !email.isEmpty() ) {
        QString name = asUtf8( cli->mAttrs[ "cn" ].first() ).trimmed();
        if ( name.isEmpty() ) {
          result << email;
        } else {
          result << name + " <" + email + ">";
        }
      }
    }
    cli = static_cast<ContactListItem*>( cli->nextSibling() );
  }

  return result.join( ", " );
}

void LdapSearchDialog::slotButtonClicked(int button)
{
  switch ( button ) {
  case Help:
    KToolInvocation::self()->invokeHelp( "ldap-queries" );
    break;
  case User1:
    mResultListView->selectAll( false );
    break;
  case User2:
    mResultListView->selectAll( true );
    break;
  case User3:
    emit addresseesAdded();
    break;
  default:
    KDialog::slotButtonClicked(button);
  }
}

#include "ldapsearchdialog.moc"
