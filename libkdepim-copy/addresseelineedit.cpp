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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "addresseelineedit.h"

#include <kabc/distributionlist.h>
#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>

#include <kcompletionbox.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstaticdeleter.h>
#include <kstdaccel.h>
#include <kurldrag.h>
#include <klocale.h>

#include "completionordereditor.h"
#include "ldapclient.h"

#include <qpopupmenu.h>
#include <qapplication.h>
#include <qobject.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qclipboard.h>
#include "resourceabc.h"

using namespace KPIM;

KCompletion * AddresseeLineEdit::s_completion = 0L;
KPIM::CompletionItemsMap* AddresseeLineEdit::s_completionItemMap = 0L;
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

// needs to be unique, but the actual name doesn't matter much
static QCString newLineEditDCOPObjectName()
{
    static int s_count = 0;
    QCString name( "KPIM::AddresseeLineEdit" );
    if ( s_count++ ) {
      name += '-';
      name += QCString().setNum( s_count );
    }
    return name;
}

AddresseeLineEdit::AddresseeLineEdit( QWidget* parent, bool useCompletion,
                                      const char *name )
  : ClickLineEdit( parent, QString::null, name ), DCOPObject( newLineEditDCOPObjectName() )
{
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
    s_completion->setOrder( KCompletion::Weighted );
    s_completion->setIgnoreCase( true );

    completionItemsDeleter.setObject( s_completionItemMap, new KPIM::CompletionItemsMap() );
  }

//  connect( s_completion, SIGNAL( match( const QString& ) ),
//           this, SLOT( slotMatched( const QString& ) ) );

  if ( m_useCompletion ) {
    if ( !s_LDAPTimer ) {
      ldapTimerDeleter.setObject( s_LDAPTimer, new QTimer );
      ldapSearchDeleter.setObject( s_LDAPSearch, new KPIM::LdapSearch );
      ldapTextDeleter.setObject( s_LDAPText, new QString );
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
        kdError() << "AddresseeLineEdit: connection to orderChanged() failed" << endl;

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

  //kdDebug(5300) << "     AddresseeLineEdit::insert( \"" << t << "\" )" << endl;

  QString newText = t.stripWhiteSpace();
  if ( newText.isEmpty() )
    return;

  // remove newlines in the to-be-pasted string as well as an eventual
  // mailto: protocol
  newText.replace( QRegExp("\r?\n"), ", " );

  if ( newText.startsWith("mailto:") ) {
    KURL url( newText );
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
  int pos = cursorPosition();

  if ( hasSelectedText() ) {
    // Cut away the selection.
    start_sel = selectionStart();
      pos = start_sel;
    contents = contents.left( start_sel ) + contents.mid( start_sel + selectedText().length() );
  }

  int eot = contents.length();
  while ((eot > 0) && contents[ eot - 1 ].isSpace() ) eot--;
  if ( eot == 0 )
    contents = QString::null;
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
       && e->button() == MidButton ) {
    m_smartPaste = true;
  }

  KLineEdit::mouseReleaseEvent( e );
  m_smartPaste = false;
}

void AddresseeLineEdit::dropEvent( QDropEvent *e )
{
  KURL::List uriList;
  if ( !isReadOnly()
       && KURLDrag::canDecode(e) && KURLDrag::decode( e, uriList ) ) {
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
    for ( KURL::List::Iterator it = uriList.begin();
          it != uriList.end(); ++it ) {
      if ( !contents.isEmpty() )
        contents.append( ", " );
      KURL u( *it );
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

  if ( s_addressesDirty )
    loadContacts(); // read from local address book

  // cursor at end of string - or Ctrl+T pressed for substring completion?
  if ( ctrlT ) {
    const QStringList completions = s_completion->substringCompletion( m_searchString );

    if ( completions.count() == 1 ) {
      setText( m_searchString + completions.first() );
      setEdited( true );
    }

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
      //m_previousAddresses = prevAddr;
      QStringList items = s_completion->allMatches( m_searchString );
      items += s_completion->allMatches( "\"" + m_searchString );
      uint beforeDollarCompletionCount = items.count();

      if ( m_searchString.find( ' ' ) == -1 ) // one word, possibly given name
        items += s_completion->allMatches( "$$" + m_searchString );

      if ( items.isEmpty() ) {
        setCompletedItems( items, false );
      } else {
        if ( items.count() > beforeDollarCompletionCount ) {
          // remove the '$$whatever$' part
          for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it ) {
            int pos = (*it).find( '$', 2 );
            if ( pos < 0 ) // ???
              continue;
            (*it) = (*it).mid( pos + 1 );
          }
        }

        bool autoSuggest = (mode != KGlobalSettings::CompletionPopupAuto);
        setCompletedItems( items, autoSuggest );

        if ( !autoSuggest ) {
          int index = items.first().find( m_searchString );
          QString newText = m_previousAddresses + items.first().mid( index );
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
  setText( m_previousAddresses + completion );
  setEdited( true );
  cursorAtEnd();
//  slotMatched( m_previousAddresses + completion );
}

void AddresseeLineEdit::loadContacts()
{
  s_completion->clear();
  s_addressesDirty = false;
  //m_contactMap.clear();

  QApplication::setOverrideCursor( KCursor::waitCursor() ); // loading might take a while

  KConfig config( "kpimcompletionorder" ); // The weights for non-imap kabc resources is there.
  config.setGroup( "CompletionWeights" );

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  // Can't just use the addressbook's iterator, we need to know which subresource
  // is behind which contact.
  QPtrList<KABC::Resource> resources( addressBook->resources() );
  for( QPtrListIterator<KABC::Resource> resit( resources ); *resit; ++resit ) {
    KABC::Resource* resource = *resit;
    KPIM::ResourceABC* resabc = dynamic_cast<ResourceABC *>( resource );
    if ( resabc ) { // IMAP KABC resource; need to associate each contact with the subresource
      const QMap<QString, QString> uidToResourceMap = resabc->uidToResourceMap();
      KABC::Resource::Iterator it;
      for ( it = resource->begin(); it != resource->end(); ++it ) {
        QString uid = (*it).uid();
        QMap<QString, QString>::const_iterator wit = uidToResourceMap.find( uid );
        int weight = ( wit != uidToResourceMap.end() ) ? resabc->subresourceCompletionWeight( *wit ) : 80;
        //kdDebug(5300) << (*it).fullEmail() << " subres=" << *wit << " weight=" << weight << endl;
        addContact( *it, weight );
      }
    } else { // KABC non-imap resource
      int weight = config.readNumEntry( resource->identifier(), 60 );
      KABC::Resource::Iterator it;
      for ( it = resource->begin(); it != resource->end(); ++it )
        addContact( *it, weight );
    }
  }

  int weight = config.readNumEntry( "DistributionLists", 60 );
  KABC::DistributionListManager manager( addressBook );
  manager.load();
  const QStringList distLists = manager.listNames();
  QStringList::const_iterator listIt;
  for ( listIt = distLists.begin(); listIt != distLists.end(); ++listIt ) {
    s_completion->addItem( (*listIt).simplifyWhiteSpace(), weight );
  }

  QApplication::restoreOverrideCursor();

  if ( !m_addressBookConnected ) {
    connect( addressBook, SIGNAL( addressBookChanged( AddressBook* ) ), SLOT( loadContacts() ) );
    m_addressBookConnected = true;
  }
}

void AddresseeLineEdit::addContact( const KABC::Addressee& addr, int weight )
{
  //m_contactMap.insert( addr.realName(), addr );
  const QStringList emails = addr.emails();
  QStringList::ConstIterator it;
  for ( it = emails.begin(); it != emails.end(); ++it ) {
    int len = (*it).length();
    if ( len == 0 ) continue;
    if( '\0' == (*it)[len-1] )
      --len;
    const QString tmp = (*it).left( len );
    const QString fullEmail = addr.fullEmail( tmp );
    //kdDebug(5300) << "AddresseeLineEdit::addContact() \"" << fullEmail << "\" weight=" << weight << endl;
    addCompletionItem( fullEmail.simplifyWhiteSpace(), weight );
    // Try to guess the last name: if found, we add an extra
    // entry to the list to make sure completion works even
    // if the user starts by typing in the last name.
    QString name( addr.realName().simplifyWhiteSpace() );
    if( name.endsWith("\"") )
      name.truncate( name.length()-1 );
    if( name.startsWith("\"") )
      name = name.mid( 1 );

    // While we're here also add "email (full name)" for completion on the email
    if ( !name.isEmpty() )
      addCompletionItem( addr.preferredEmail() + " (" + name + ")", weight );
    
    if ( name.find( ',' ) ) return; // probably already of the form "Lastname, Firstname"

    bool bDone = false;
    int i = -1;
    while( ( i = name.findRev(' ') > 1 ) && !bDone ) {
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
        //kdDebug(5300) << "AddresseeLineEdit::addContact() added extra \"" << sExtraEntry.simplifyWhiteSpace() << "\" weight=" << weight << endl;
        addCompletionItem( sExtraEntry.simplifyWhiteSpace(), weight );
        bDone = true;
      }
      if( !bDone ){
        name.truncate( i );
        if( name.endsWith("\"") )
          name.truncate( name.length()-1 );
      }
    }
  }
}

void AddresseeLineEdit::addCompletionItem( const QString& string, int weight )
{
  // Check if there is an exact match for item already, and use the max weight if so.
  // Since there's no way to get the information from KCompletion, we have to keep our own QMap
  CompletionItemsMap::iterator it = s_completionItemMap->begin();
  if ( it != s_completionItemMap->end() ) {
    weight = QMAX( *it, weight );
    *it = weight;
  } else {
    s_completionItemMap->insert( string, weight );
  }

  s_completion->addItem( string, weight );
}

void AddresseeLineEdit::slotStartLDAPLookup()
{
  if ( !s_LDAPSearch->isAvailable() || s_LDAPLineEdit != this )
    return;

  startLoadingLDAPEntries();
}

void AddresseeLineEdit::stopLDAPLookup()
{
  s_LDAPSearch->cancelSearch();
  s_LDAPLineEdit = NULL;
  s_completionItemMap->clear();
}

void AddresseeLineEdit::startLoadingLDAPEntries()
{
  QString s( *s_LDAPText );
  // TODO cache last?
  QString prevAddr;
  int n = s.findRev( ',' );
  if ( n >= 0 ) {
    prevAddr = s.left( n + 1 ) + ' ';
    s = s.mid( n + 1, 255 ).stripWhiteSpace();
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
    addContact( addr, (*it).completionWeight );
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
          QListBoxItem* item = completionBox->findItem( currentSelection, Qt::ExactMatch );
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
        if ( completionBox && completionBox->isVisible() )
            completionBox->hide();
    }
}

QPopupMenu* AddresseeLineEdit::createPopupMenu()
{
  QPopupMenu *menu = KLineEdit::createPopupMenu();
  if ( !menu )
    return 0;

  if ( m_useCompletion )
    menu->insertItem( i18n( "Edit Completion Order..." ),
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
    m_searchString = m_searchString.mid( n ).stripWhiteSpace();
  }
  else
  {
    m_previousAddresses = QString::null;
  }
  doCompletion( false );
}

#include "addresseelineedit.moc"
