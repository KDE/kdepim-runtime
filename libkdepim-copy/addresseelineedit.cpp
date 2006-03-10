/*
    This file is part of libkdepim.
    Copyright (c) 2002 Helge Deller <deller@gmx.de>
                  2002 Lubos Lunak <llunak@suse.cz>
                  2001,2003 Carsten Pfeiffer <pfeiffer@kde.org>
                  2001 Waldo Bastian <bastian@kde.org>
                  2004 Daniel Molkentin <danimo@klaralvdalens-datakonsult.se>
                  2004 Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "addresseelineedit.h"

#include "resourceabc.h"
#include "completionordereditor.h"
#include "ldapclient.h"

#include <config.h>

#ifdef KDEPIM_NEW_DISTRLISTS
#include "distributionlist.h"
#else
#include <kabc/distributionlist.h>
#endif

#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>
#include <libemailfunctions/email.h>

#include <kcompletionbox.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstaticdeleter.h>
#include <kstdaccel.h>
#include <kurl.h>
#include <klocale.h>
#include <kdatastream.h>

#include <qapplication.h>
#include <qobject.h>
#include <qregexp.h>
#include <qevent.h>
#include <q3dragobject.h>
#include <qclipboard.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QMenu>

using namespace KPIM;

KCompletion * AddresseeLineEdit::s_completion = 0L;
KPIM::CompletionItemsMap* AddresseeLineEdit::s_completionItemMap = 0L;
QStringList* AddresseeLineEdit::s_completionSources = 0L;
bool AddresseeLineEdit::s_addressesDirty = false;
QTimer* AddresseeLineEdit::s_LDAPTimer = 0L;
KPIM::LdapSearch* AddresseeLineEdit::s_LDAPSearch = 0L;
QString* AddresseeLineEdit::s_LDAPText = 0L;
AddresseeLineEdit* AddresseeLineEdit::s_LDAPLineEdit = 0L;

static KStaticDeleter<KCompletion> completionDeleter;
static KStaticDeleter<KPIM::CompletionItemsMap> completionItemsDeleter;
static KStaticDeleter<QTimer> ldapTimerDeleter;
static KStaticDeleter<KPIM::LdapSearch> ldapSearchDeleter;
static KStaticDeleter<QString> ldapTextDeleter;
static KStaticDeleter<KConfig> configDeleter;
static KStaticDeleter<QStringList> completionSourcesDeleter;

// needs to be unique, but the actual name doesn't matter much
static QByteArray newLineEditDCOPObjectName()
{
    static int s_count = 0;
    QByteArray name( "KPIM::AddresseeLineEdit" );
    if ( s_count++ ) {
      name += '-';
      name += QByteArray().setNum( s_count );
    }
    return name;
}

static const QString s_completionItemIndentString = "     ";

AddresseeLineEdit::AddresseeLineEdit( QWidget* parent, bool useCompletion,
                                      const char *name )
  : KLineEdit( parent ), DCOPObject( newLineEditDCOPObjectName() )
{
  setObjectName(name);
  setClickMessage("");
  m_useCompletion = useCompletion;
  m_completionInitialized = false;
  m_smartPaste = false;
  m_addressBookConnected = false;

  init();

  if ( m_useCompletion )
    s_addressesDirty = true;
}


void AddresseeLineEdit::init()
{
  if ( !s_completion ) {
    completionDeleter.setObject( s_completion, new KCompletion() );
    s_completion->setOrder( completionOrder() );
    s_completion->setIgnoreCase( true );

    completionItemsDeleter.setObject( s_completionItemMap, new KPIM::CompletionItemsMap() );
    completionSourcesDeleter.setObject( s_completionSources, new QStringList() );
  }

//  connect( s_completion, SIGNAL( match( const QString& ) ),
//           this, SLOT( slotMatched( const QString& ) ) );

  if ( m_useCompletion ) {
    if ( !s_LDAPTimer ) {
      ldapTimerDeleter.setObject( s_LDAPTimer, new QTimer );
      ldapSearchDeleter.setObject( s_LDAPSearch, new KPIM::LdapSearch );
      ldapTextDeleter.setObject( s_LDAPText, new QString );

      /* Add completion sources for all ldap server, 0 to n. Added first so 
       * that they map to the ldapclient::clientNumber() */
      QList< LdapClient* > clients =  s_LDAPSearch->clients();
      for ( QList<LdapClient*>::iterator it = clients.begin(); it != clients.end(); ++it ) {
        addCompletionSource( "LDAP server: " + (*it)->server().host() );
      }
    }
    if ( !m_completionInitialized ) {
      setCompletionObject( s_completion, false );
      connect( this, SIGNAL( completion( const QString& ) ),
          this, SLOT( slotCompletion() ) );

      KCompletionBox *box = completionBox();
      connect( box, SIGNAL( highlighted( const QString& ) ),
          this, SLOT( slotPopupCompletion( const QString& ) ) );
      connect( box, SIGNAL( userCancelled( const QString& ) ),
          SLOT( slotUserCancelled( const QString& ) ) );

      // The emitter is always called KPIM::IMAPCompletionOrder by contract
      if ( !connectDCOPSignal( 0, "KPIM::IMAPCompletionOrder", "orderChanged()",
            "slotIMAPCompletionOrderChanged()", false ) )
        kError() << "AddresseeLineEdit: connection to orderChanged() failed" << endl;

      connect( s_LDAPTimer, SIGNAL( timeout() ), SLOT( slotStartLDAPLookup() ) );
      connect( s_LDAPSearch, SIGNAL( searchData( const KPIM::LdapResultList& ) ),
          SLOT( slotLDAPSearchData( const KPIM::LdapResultList& ) ) );

      m_completionInitialized = true;
    }
  }
}

AddresseeLineEdit::~AddresseeLineEdit()
{
  if ( s_LDAPSearch && s_LDAPLineEdit == this )
    stopLDAPLookup();
}

void AddresseeLineEdit::setFont( const QFont& font )
{
  KLineEdit::setFont( font );
  if ( m_useCompletion )
    completionBox()->setFont( font );
}

void AddresseeLineEdit::keyPressEvent( QKeyEvent *e )
{
  bool accept = false;

  if ( KStdAccel::shortcut( KStdAccel::SubstringCompletion ).contains( KKey( e ) ) ) {
    doCompletion( true );
    accept = true;
  } else if ( KStdAccel::shortcut( KStdAccel::TextCompletion ).contains( KKey( e ) ) ) {
    int len = text().length();

    if ( len == cursorPosition() ) { // at End?
      doCompletion( true );
      accept = true;
    }
  }

  if ( !accept )
    KLineEdit::keyPressEvent( e );

  if ( e->isAccepted() ) {
    if ( m_useCompletion && s_LDAPTimer != NULL ) {
      if ( *s_LDAPText != text() || s_LDAPLineEdit != this )
        stopLDAPLookup();

      *s_LDAPText = text();
      s_LDAPLineEdit = this;
      s_LDAPTimer->start( 500, true );
    }
  }
}

void AddresseeLineEdit::insert( const QString &t )
{
  if ( !m_smartPaste ) {
    KLineEdit::insert( t );
    return;
  }

  //kDebug(5300) << "     AddresseeLineEdit::insert( \"" << t << "\" )" << endl;

  QString newText = t.trimmed();
  if ( newText.isEmpty() )
    return;

  // remove newlines in the to-be-pasted string
  QStringList lines = newText.split( QRegExp("\r?\n"), QString::SkipEmptyParts );
  for ( QStringList::iterator it = lines.begin();
       it != lines.end(); ++it ) {
    // remove trailing commas and whitespace
    (*it).remove( QRegExp(",?\\s*$") );
  }
  newText = lines.join( ", " );

  if ( newText.startsWith("mailto:") ) {
    KUrl url( newText );
    newText = url.path();
  }
  else if ( newText.find(" at ") != -1 ) {
    // Anti-spam stuff
    newText.replace( " at ", "@" );
    newText.replace( " dot ", "." );
  }
  else if ( newText.find("(at)") != -1 ) {
    newText.replace( QRegExp("\\s*\\(at\\)\\s*"), "@" );
  }

  QString contents = text();
  int start_sel = 0;
  int end_sel = 0;
  int pos = cursorPosition( );
  if ( getSelection( &start_sel, &end_sel ) ) {
    // Cut away the selection.
    if ( pos > end_sel )
      pos -= (end_sel - start_sel);
    else if ( pos > start_sel )
      pos = start_sel;
    contents = contents.left( start_sel ) + contents.right( end_sel + 1 );
  }

  int eot = contents.length();
  while ((eot > 0) && contents[ eot - 1 ].isSpace() ) eot--;
  if ( eot == 0 )
    contents.clear();
  else if ( pos >= eot ) {
    if ( contents[ eot - 1 ] == ',' )
      eot--;
    contents.truncate( eot );
    contents += ", ";
    pos = eot + 2;
  }

  contents = contents.left( pos ) + newText + contents.mid( pos );
  setText( contents );
  setEdited( true );
  setCursorPosition( pos + newText.length() );
}

void AddresseeLineEdit::setText( const QString & text )
{
  KLineEdit::setText( text.trimmed() );
}

void AddresseeLineEdit::paste()
{
  if ( m_useCompletion )
    m_smartPaste = true;

  KLineEdit::paste();
  m_smartPaste = false;
}

void AddresseeLineEdit::mouseReleaseEvent( QMouseEvent *e )
{
  // reimplemented from QLineEdit::mouseReleaseEvent()
  if ( m_useCompletion
       && QApplication::clipboard()->supportsSelection()
       && !isReadOnly()
       && e->button() == Qt::MidButton ) {
    m_smartPaste = true;
  }

  KLineEdit::mouseReleaseEvent( e );
  m_smartPaste = false;
}

void AddresseeLineEdit::dropEvent( QDropEvent *e )
{
  if ( !isReadOnly() ) {
    KUrl::List uriList = KUrl::List::fromMimeData( e->mimeData() );
    if ( !uriList.isEmpty() ) {
      QString contents = text();
      // remove trailing white space and comma
      int eot = contents.length();
      while ( ( eot > 0 ) && contents[ eot - 1 ].isSpace() )
        eot--;
      if ( eot == 0 )
        contents = QString::null;
      else if ( contents[ eot - 1 ] == ',' ) {
        eot--;
        contents.truncate( eot );
      }
      bool mailtoURL = false;
      // append the mailto URLs
      for ( KUrl::List::Iterator it = uriList.begin();
            it != uriList.end(); ++it ) {
        if ( !contents.isEmpty() )
          contents.append( ", " );
        KUrl u( *it );
        if ( u.protocol() == "mailto" ) {
          mailtoURL = true;
          contents.append( (*it).path() );
        }
      }
      if ( mailtoURL ) {
        setText( contents );
        setEdited( true );
        return;
      }
    }
  }

  if ( m_useCompletion )
    m_smartPaste = true;
  QLineEdit::dropEvent( e );
  m_smartPaste = false;
}

void AddresseeLineEdit::cursorAtEnd()
{
  setCursorPosition( text().length() );
}

void AddresseeLineEdit::enableCompletion( bool enable )
{
  m_useCompletion = enable;
}

void AddresseeLineEdit::doCompletion( bool ctrlT )
{
  if ( !m_useCompletion )
    return;

  if ( s_addressesDirty ) {
    loadContacts(); // read from local address book
    s_completion->setOrder( completionOrder() );
  }

  // cursor at end of string - or Ctrl+T pressed for substring completion?
  if ( ctrlT ) {
    const QStringList completions = getAdjustedCompletionItems( false );

    if ( completions.count() > 1 )
      ; //m_previousAddresses = prevAddr;
    else if ( completions.count() == 1 )
      setText( m_previousAddresses + completions.first().trimmed() );

    setCompletedItems( completions, true ); // this makes sure the completion popup is closed if no matching items were found

    cursorAtEnd();
    return;
  }

  KGlobalSettings::Completion  mode = completionMode();

  switch ( mode ) {
    case KGlobalSettings::CompletionPopupAuto:
    {
      if ( m_searchString.isEmpty() )
        break;
    }

    case KGlobalSettings::CompletionPopup:
    {
      const QStringList items = getAdjustedCompletionItems( true );
      if ( items.isEmpty() ) {
        setCompletedItems( items, false );
      }
      else {
        const bool autoSuggest = (mode != KGlobalSettings::CompletionPopupAuto);
        setCompletedItems( items, autoSuggest );

        if ( !autoSuggest ) {
          const int index = items.first().find( m_searchString );
          QString newText = m_previousAddresses + items.first().mid( index ).trimmed();
          setUserSelection( false );
          setCompletedText( newText, true );
        }
      }
      break;
    }

    case KGlobalSettings::CompletionShell:
    {
      QString match = s_completion->makeCompletion( m_searchString );
      if ( !match.isNull() && match != m_searchString ) {
        setText( m_previousAddresses + match );
        setEdited( true );
        cursorAtEnd();
      }
      break;
    }

    case KGlobalSettings::CompletionMan: // Short-Auto in fact
    case KGlobalSettings::CompletionAuto:
    {
      if ( !m_searchString.isEmpty() ) {
        QString match = s_completion->makeCompletion( m_searchString );
        if ( !match.isNull() && match != m_searchString ) {
          QString adds = m_previousAddresses + match;
          setCompletedText( adds );
        }
        break;
      }
    }

    case KGlobalSettings::CompletionNone:
    default: // fall through
      break;
  }
}

void AddresseeLineEdit::slotPopupCompletion( const QString& completion )
{
  setText( m_previousAddresses + completion.trimmed() );
  cursorAtEnd();
//  slotMatched( m_previousAddresses + completion );
}

void AddresseeLineEdit::loadContacts()
{
  s_completion->clear();
  s_completionItemMap->clear();
  s_addressesDirty = false;
  //m_contactMap.clear();

  QApplication::setOverrideCursor( KCursor::waitCursor() ); // loading might take a while

  KConfig config( "kpimcompletionorder" ); // The weights for non-imap kabc resources is there.
  config.setGroup( "CompletionWeights" );

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  // Can't just use the addressbook's iterator, we need to know which subresource
  // is behind which contact.
  QList<KABC::Resource*> resources( addressBook->resources() );
  QListIterator<KABC::Resource*> resit( resources );
  while ( resit.hasNext() ) {
    KABC::Resource* resource = resit.next();
    KPIM::ResourceABC* resabc = dynamic_cast<ResourceABC *>( resource );
    if ( resabc ) { // IMAP KABC resource; need to associate each contact with the subresource
      const QMap<QString, QString> uidToResourceMap = resabc->uidToResourceMap();
      KABC::Resource::Iterator it;
      for ( it = resource->begin(); it != resource->end(); ++it ) {
        QString uid = (*it).uid();
        QMap<QString, QString>::const_iterator wit = uidToResourceMap.find( uid );
        const QString subresourceLabel = resabc->subresourceLabel( *wit );
        int idx = s_completionSources->indexOf( subresourceLabel );
        if ( idx == -1 ) {
          s_completionSources->append( subresourceLabel );
          idx = s_completionSources->size() -1;
        }
        int weight = ( wit != uidToResourceMap.end() ) ? resabc->subresourceCompletionWeight( *wit ) : 80;
        //kDebug(5300) << (*it).fullEmail() << " subres=" << *wit << " weight=" << weight << endl;
        addContact( *it, weight, idx );
      }
    } else { // KABC non-imap resource
      int weight = config.readEntry( resource->identifier(), 60 );
      s_completionSources->append( resource->resourceName() );
      KABC::Resource::Iterator it;
      for ( it = resource->begin(); it != resource->end(); ++it )
        addContact( *it, weight, s_completionSources->size()-1 );
    }
  }

#ifndef KDEPIM_NEW_DISTRLISTS // new distr lists are normal contact, already done above
  int weight = config.readEntry( "DistributionLists", 60 );
  KABC::DistributionListManager manager( addressBook );
  manager.load();
  const QStringList distLists = manager.listNames();
  QStringList::const_iterator listIt;
  int idx = addCompletionSource( i18n( "Distribution Lists" ) );
  for ( listIt = distLists.begin(); listIt != distLists.end(); ++listIt ) {
    addCompletionItem( (*listIt).simplified(), weight, idx );
  }
#endif

  QApplication::restoreOverrideCursor();

  if ( !m_addressBookConnected ) {
    connect( addressBook, SIGNAL( addressBookChanged( AddressBook* ) ), SLOT( loadContacts() ) );
    m_addressBookConnected = true;
  }
}

void AddresseeLineEdit::addContact( const KABC::Addressee& addr, int weight, int source )
{
#ifdef KDEPIM_NEW_DISTRLISTS
  if ( KPIM::DistributionList::isDistributionList( addr ) ) {
    //kDebug(5300) << "AddresseeLineEdit::addContact() distribution list \"" << addr.formattedName() << "\" weight=" << weight << endl;
    addCompletionItem( addr.formattedName(), weight, source );
    return;
  }
#endif
  //m_contactMap.insert( addr.realName(), addr );
  const QStringList emails = addr.emails();
  QStringList::ConstIterator it;
  for ( it = emails.begin(); it != emails.end(); ++it ) {

    if ( addr.givenName().isEmpty() && addr.familyName().isEmpty() ) {
      addCompletionItem( addr.fullEmail( (*it) ), weight, source ); // use whatever is there
    } else {
      const QString byFirstName= KPIM::quoteNameIfNecessary( addr.givenName() + " " + addr.familyName() ) + " <" + (*it) + ">";
      const QString byLastName= "\"" + addr.familyName() + ", " + addr.givenName() + "\" "  + "<" + (*it) + ">";
      const QString byEmail= (*it) + " (" + KPIM::quoteNameIfNecessary( addr.realName() ) + ")";
      addCompletionItem( byFirstName, weight, source );
      addCompletionItem( byLastName, weight, source );
      addCompletionItem( byEmail, weight, source );
    }

#if 0
    int len = (*it).length();
    if ( len == 0 ) continue;
    if( '\0' == (*it)[len-1] )
      --len;
    const QString tmp = (*it).left( len );
    const QString fullEmail = addr.fullEmail( tmp );
    //kDebug(5300) << "AddresseeLineEdit::addContact() \"" << fullEmail << "\" weight=" << weight << endl;
    addCompletionItem( fullEmail.simplified(), weight, source );
    // Try to guess the last name: if found, we add an extra
    // entry to the list to make sure completion works even
    // if the user starts by typing in the last name.
    QString name( addr.realName().simplified() );
    if( name.endsWith("\"") )
      name.truncate( name.length()-1 );
    if( name.startsWith("\"") )
      name = name.mid( 1 );

    // While we're here also add "email (full name)" for completion on the email
    if ( !name.isEmpty() )
      addCompletionItem( addr.preferredEmail() + " (" + name + ")", weight, source );

    bool bDone = false;
    int i = -1;
    while( ( i = name.findRev(' ') ) > 1 && !bDone ) {
      QString sLastName( name.mid( i+1 ) );
      if( ! sLastName.isEmpty() &&
            2 <= sLastName.length() &&   // last names must be at least 2 chars long
          ! sLastName.endsWith(".") ) { // last names must not end with a dot (like "Jr." or "Sr.")
        name.truncate( i );
        if( !name.isEmpty() ){
          sLastName.prepend( "\"" );
          sLastName.append( ", " + name + "\" <" );
        }
        QString sExtraEntry( sLastName );
        sExtraEntry.append( tmp.isEmpty() ? addr.preferredEmail() : tmp );
        sExtraEntry.append( ">" );
        //kDebug(5300) << "AddresseeLineEdit::addContact() added extra \"" << sExtraEntry.simplified() << "\" weight=" << weight << endl;
        addCompletionItem( sExtraEntry.simplified(), weight, source );
        bDone = true;
      }
      if( !bDone ) {
        name.truncate( i );
        if( name.endsWith("\"") )
          name.truncate( name.length()-1 );
      }
    }
#endif
  }
}

void AddresseeLineEdit::addCompletionItem( const QString& string, int weight, int completionItemSource )
{
  // Check if there is an exact match for item already, and use the max weight if so.
  // Since there's no way to get the information from KCompletion, we have to keep our own QMap
  CompletionItemsMap::iterator it = s_completionItemMap->find( string );
  if ( it != s_completionItemMap->end() ) {
    weight = qMax( ( *it ).first, weight );
    ( *it ).first = weight;
  } else {
    s_completionItemMap->insert( string, qMakePair( weight, completionItemSource ) );
  }
  s_completion->addItem( string, weight );
}

void AddresseeLineEdit::slotStartLDAPLookup()
{
  if ( !s_LDAPSearch->isAvailable() ) {
    return;
  }
  if (  s_LDAPLineEdit != this )
    return;

  startLoadingLDAPEntries();
}

void AddresseeLineEdit::stopLDAPLookup()
{
  s_LDAPSearch->cancelSearch();
  s_LDAPLineEdit = NULL;
}

void AddresseeLineEdit::startLoadingLDAPEntries()
{
  QString s( *s_LDAPText );
  // TODO cache last?
  QString prevAddr;
  int n = s.findRev( ',' );
  if ( n >= 0 ) {
    prevAddr = s.left( n + 1 ) + ' ';
    s = s.mid( n + 1, 255 ).trimmed();
  }

  if ( s.isEmpty() )
    return;

  loadContacts(); // TODO reuse these?
  s_LDAPSearch->startSearch( s );
}

void AddresseeLineEdit::slotLDAPSearchData( const KPIM::LdapResultList& adrs )
{
  if ( s_LDAPLineEdit != this )
    return;

  for ( KPIM::LdapResultList::ConstIterator it = adrs.begin(); it != adrs.end(); ++it ) {
    KABC::Addressee addr;
    addr.setNameFromString( (*it).name );
    addr.setEmails( (*it).email );
    
    addContact( addr, (*it).completionWeight, (*it ).clientNumber  );
  }

  if ( hasFocus() || completionBox()->hasFocus() )
    if ( completionMode() != KGlobalSettings::CompletionNone )
      doCompletion( false );
}

void AddresseeLineEdit::setCompletedItems( const QStringList& items, bool autoSuggest )
{
    KCompletionBox* completionBox = this->completionBox();

    if ( !items.isEmpty() &&
         !(items.count() == 1 && m_searchString == items.first()) )
    {
        if ( completionBox->isVisible() )
        {
          bool wasSelected = completionBox->isSelected( completionBox->currentItem() );
          const QString currentSelection = completionBox->currentText();
          completionBox->setItems( items );
          Q3ListBoxItem* item = completionBox->findItem( currentSelection, Q3ListBox::ExactMatch );
          if ( item )
          {
            completionBox->blockSignals( true );
            completionBox->setCurrentItem( item );
            completionBox->setSelected( item, wasSelected );
            completionBox->blockSignals( false );
          }
        }
        else // completion box not visible yet -> show it
        {
          if ( !m_searchString.isEmpty() )
            completionBox->setCancelledText( m_searchString );
          completionBox->setItems( items );
          completionBox->popup();
          // we have to install the event filter after popup(), since that 
          // calls show(), and that's where KCompletionBox installs its filter.
          // We want to be first, though, so do it now.
          if ( s_completion->order() == KCompletion::Weighted )
            qApp->installEventFilter( this );
        }

        if ( autoSuggest )
        {
            int index = items.first().find( m_searchString );
            QString newText = items.first().mid( index );
            setUserSelection(false);
            setCompletedText(newText,true);
        }
    }
    else
    {
        if ( completionBox && completionBox->isVisible() ) {
            completionBox->hide();
        }
    }
}

QMenu* AddresseeLineEdit::createStandardContextMenu()
{
  QMenu *menu = KLineEdit::createStandardContextMenu();
  if ( !menu )
    return 0;

  if ( m_useCompletion )
    menu->addAction( i18n( "Configure Completion Order..." ),
                      this, SLOT( slotEditCompletionOrder() ) );
  return menu;
}

void AddresseeLineEdit::slotEditCompletionOrder()
{
  init(); // for s_LDAPSearch
  CompletionOrderEditor editor( s_LDAPSearch, this );
  editor.exec();
}

void KPIM::AddresseeLineEdit::slotIMAPCompletionOrderChanged()
{
  if ( m_useCompletion )
    s_addressesDirty = true;
}

void KPIM::AddresseeLineEdit::slotUserCancelled( const QString& cancelText )
{
  if ( s_LDAPSearch && s_LDAPLineEdit == this )
    stopLDAPLookup();
  userCancelled( cancelText ); // in KLineEdit
}

void KPIM::AddresseeLineEdit::slotCompletion()
{
  // Called by KLineEdit's keyPressEvent -> new text, update search string
  m_searchString = text();
  int n = m_searchString.findRev(',');
  if ( n >= 0 ) {
    ++n; // Go past the ","

    int len = m_searchString.length();

    // Increment past any whitespace...
    while ( n < len && m_searchString[ n ].isSpace() )
      ++n;

    m_previousAddresses = m_searchString.left( n );
    m_searchString = m_searchString.mid( n ).trimmed();
  }
  else
  {
    m_previousAddresses.clear();
  }
  if ( completionBox() )
    completionBox()->setCancelledText( m_searchString );
  doCompletion( false );
}

// not cached, to make sure we get an up-to-date value when it changes
KCompletion::CompOrder KPIM::AddresseeLineEdit::completionOrder()
{
  KConfig config( "kpimcompletionorder" );
  config.setGroup( "General" );
  const QString order = config.readEntry( "CompletionOrder", "Weighted" );

  if ( order == "Weighted" )
    return KCompletion::Weighted;
  else
    return KCompletion::Sorted;
}

int KPIM::AddresseeLineEdit::addCompletionSource( const QString &source )
{
  s_completionSources->append( source );
  return s_completionSources->size()-1;
}

bool KPIM::AddresseeLineEdit::eventFilter(QObject *obj, QEvent *e)
{
#warning Port me!
  // FIXME Temporary avoid the constructor in the IF because of endless loops!
  //if ( obj == completionBox() ) {
  if (  0 && obj == completionBox() ) {
    if ( e->type() == QEvent::MouseButtonPress
      || e->type() == QEvent::MouseMove
      || e->type() == QEvent::MouseButtonRelease ) {
      QMouseEvent* me = static_cast<QMouseEvent*>( e );
      // find list box item at the event position
      Q3ListBoxItem *item = completionBox()->itemAt( me->pos() );
      if ( !item ) {
        // In the case of a mouse move outside of the box we don't want
        // the parent to fuzzy select a header by mistake.
        bool eat = e->type() == QEvent::MouseMove;
        return eat;
      }
      // avoid selection of headers on button press, or move or release while
      // a button is pressed
      if ( e->type() == QEvent::MouseButtonPress 
          || me->state() & Qt::LeftButton || me->state() & Qt::MidButton 
          || me->state() & Qt::RightButton ) {
        if ( !item->text().startsWith( s_completionItemIndentString ) ) {
          return true; // eat the event, we don't want anything to happen
        } else {
          // if we are not on one of the group heading, make sure the item
          // below or above is selected, not the heading, inadvertedly, due
          // to fuzzy auto-selection from QListBox
          completionBox()->setCurrentItem( item );
          completionBox()->setSelected( completionBox()->index( item ), true );
          if ( e->type() == QEvent::MouseMove )
            return true; // avoid fuzzy selection behavior
        }
      }
    }
  }
  if ( ( obj == this ) &&
     ( e->type() == QEvent::ShortcutOverride ) ) {
    QKeyEvent *ke = static_cast<QKeyEvent*>( e );
    if ( ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down || ke->key() == Qt::Key_Tab ) {
      ke->accept();
      return true;
    }
  }
  if ( ( obj == this ) &&
      ( e->type() == QEvent::KeyPress ) &&
      completionBox()->isVisible() ) {
    QKeyEvent *ke = static_cast<QKeyEvent*>( e );
    unsigned int currentIndex = completionBox()->currentItem();
    if ( ke->key() == Qt::Key_Up ) {
      //kDebug() << "EVENTFILTER: Key_Up currentIndex=" << currentIndex << endl;
      // figure out if the item we would be moving to is one we want
      // to ignore. If so, go one further
      Q3ListBoxItem *itemAbove = completionBox()->item( currentIndex - 1 );
      if ( itemAbove && !itemAbove->text().startsWith( s_completionItemIndentString ) ) {
        // there is a header above is, check if there is even further up
        // and if so go one up, so it'll be selected
        if ( currentIndex > 1 && completionBox()->item( currentIndex - 2 ) ) {
          //kDebug() << "EVENTFILTER: Key_Up -> skipping " << currentIndex - 1 << endl;
          completionBox()->setCurrentItem( itemAbove->prev() );
          completionBox()->setSelected( currentIndex - 2, true );
        } else if ( currentIndex == 1 ) {
            // nothing to skip to, let's stay where we are, but make sure the
            // first header becomes visible, if we are the first real entry
            completionBox()->ensureVisible( 0, 0 );
        }
        return true;
      }
    } else if ( ke->key() == Qt::Key_Down || ke->key() == Qt::Key_Tab ) {
      // same strategy for downwards
      //kDebug() << "EVENTFILTER: Key_Down. currentIndex=" << currentIndex << endl;
      Q3ListBoxItem *itemBelow = completionBox()->item( currentIndex + 1 );
      if ( itemBelow && !itemBelow->text().startsWith( s_completionItemIndentString ) ) {
        if ( completionBox()->item( currentIndex + 2 ) ) {
          //kDebug() << "EVENTFILTER: Key_Down -> skipping " << currentIndex+1 << endl;
          completionBox()->setCurrentItem( itemBelow->next() );
          completionBox()->setSelected( currentIndex + 2, true );
        } else {
          // nothing to skip to, let's stay where we are
        }
        return true;
      }
      // special case of the initial selection, which is unfortunately a header.
      // Setting it to selected tricks KCompletionBox into not treating is special
      // and selecting making it current, instead of the one below.
      Q3ListBoxItem *item = completionBox()->item( currentIndex );
      if ( item && !item->text().startsWith( s_completionItemIndentString )  ) {
        completionBox()->setSelected( currentIndex, true );
      }
    }
  }
  return KLineEdit::eventFilter( obj, e );
}

const QStringList KPIM::AddresseeLineEdit::getAdjustedCompletionItems( bool fullSearch )
{
  QStringList items = fullSearch ?
    s_completion->allMatches( m_searchString )
    : s_completion->substringCompletion( m_searchString );
  if ( fullSearch )
    items += s_completion->allMatches( "\"" + m_searchString );
  unsigned int beforeDollarCompletionCount = items.count();

  if ( fullSearch && m_searchString.find( ' ' ) == -1 ) // one word, possibly given name
    items += s_completion->allMatches( "$$" + m_searchString );

  // kDebug(5300) << "     AddresseeLineEdit::doCompletion() found: " << items.join(" AND ") << endl;

  int lastSourceIndex = -1;
  unsigned int i = 0;
  QMap<int, QStringList> sections;
  for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it, ++i ) {
    CompletionItemsMap::const_iterator cit = s_completionItemMap->find(*it);
    if ( cit == s_completionItemMap->end() )continue;
    int idx = (*cit).second;
    if ( s_completion->order() == KCompletion::Weighted ) {
      if ( lastSourceIndex == -1 || lastSourceIndex != idx ) {
        const QString sourceLabel(  (*s_completionSources)[idx] );
        if ( sections.find(idx) == sections.end() ) {
          items.insert( it, sourceLabel );
        }
        lastSourceIndex = idx;
      }
      (*it) = (*it).prepend( s_completionItemIndentString );
      sections[idx].append( *it );
    }
    if ( i > beforeDollarCompletionCount ) { 
      // remove the '$$whatever$' part
      int pos = (*it).find( '$', 2 );
      if ( pos < 0 ) // ???
        continue;
      (*it) = (*it).mid( pos + 1 );
    }
  }
  QStringList sortedItems;
  for ( QMap<int, QStringList>::Iterator it( sections.begin() ), end( sections.end() ); it != end; ++it ) {
    sortedItems.append( (*s_completionSources)[it.key()] );
    for ( QStringList::Iterator sit( (*it).begin() ), send( (*it).end() ); sit != send; ++sit ) {
      sortedItems.append( *sit );
    }
  }
  return sortedItems;
}
#include "addresseelineedit.moc"
