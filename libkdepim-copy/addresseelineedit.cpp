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

#include <qapplication.h>
#include <qobject.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qevent.h>
#include <qdragobject.h>

#include <kcompletionbox.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstaticdeleter.h>
#include <kstdaccel.h>
#include <kurldrag.h>

#include <kabc/distributionlist.h>
#include "ldapclient.h"
#include <kabc/stdaddressbook.h>

#include "addresseelineedit.h"

using namespace KPIM;

KCompletion * AddresseeLineEdit::s_completion = 0L;
bool AddresseeLineEdit::s_addressesDirty = false;
QTimer* AddresseeLineEdit::s_LDAPTimer = 0L;
KPIM::LdapSearch* AddresseeLineEdit::s_LDAPSearch = 0L;
QString* AddresseeLineEdit::s_LDAPText = 0L;
AddresseeLineEdit* AddresseeLineEdit::s_LDAPLineEdit = 0L;

static KStaticDeleter<KCompletion> completionDeleter;
static KStaticDeleter<QTimer> ldapTimerDeleter;
static KStaticDeleter<KPIM::LdapSearch> ldapSearchDeleter;
static KStaticDeleter<QString> ldapTextDeleter;

AddresseeLineEdit::AddresseeLineEdit( QWidget* parent, bool useCompletion,
                                      const char *name )
  : ClickLineEdit( parent, QString::null, name )
{
  m_useCompletion = useCompletion;
  m_completionInitialized = false;
  m_smartPaste = false;

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
  }

//  connect( s_completion, SIGNAL( match( const QString& ) ),
//           this, SLOT( slotMatched( const QString& ) ) );

  if ( m_useCompletion ) {
    if ( !s_LDAPTimer ) {
      ldapTimerDeleter.setObject( s_LDAPTimer, new QTimer );
      ldapSearchDeleter.setObject( s_LDAPSearch, new KPIM::LdapSearch );
      ldapTextDeleter.setObject( s_LDAPText, new QString );
    }
    connect( s_LDAPTimer, SIGNAL( timeout() ), SLOT( slotStartLDAPLookup() ) );
    connect( s_LDAPSearch, SIGNAL( searchData( const KPIM::LdapResultList& ) ),
             SLOT( slotLDAPSearchData( const KPIM::LdapResultList& ) ) );
  }

  if ( m_useCompletion && !m_completionInitialized ) {
    setCompletionObject( s_completion, false );
    connect( this, SIGNAL( completion( const QString& ) ),
             this, SLOT( slotCompletion() ) );

    KCompletionBox *box = completionBox();
    connect( box, SIGNAL( highlighted( const QString& ) ),
             this, SLOT( slotPopupCompletion( const QString& ) ) );
    connect( box, SIGNAL( userCancelled( const QString& ) ),
             SLOT( userCancelled( const QString& ) ) );

    m_completionInitialized = true;
  }
}

AddresseeLineEdit::~AddresseeLineEdit()
{
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
      if ( *s_LDAPText != text() )
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

  QString newText = t.stripWhiteSpace();
  if ( newText.isEmpty() )
    return;

  // remove newlines in the to-be-pasted string
  newText.replace( QRegExp("\r?\n"), ", " );

  QString contents = text();
  int start_sel = 0;
  int end_sel = 0;
  int pos = cursorPosition();
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
  setCursorPosition( pos + newText.length() );
}

void AddresseeLineEdit::paste()
{
  if ( m_useCompletion )
    m_smartPaste = true;

  KLineEdit::paste();
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

  QString prevAddr;

  QString s( text() );
  int n = s.findRev(',');

  if ( n >= 0 ) {
    n++; // Go past the ","

    int len = s.length();

    // Increment past any whitespace...
    while ( n < len && s[ n ].isSpace() )
      n++;

    prevAddr = s.left( n );
    s = s.mid( n, 255 ).stripWhiteSpace();
  }

  if ( s_addressesDirty )
    loadContacts(); // read from local address book

  // cursor at end of string - or Ctrl+T pressed for substring completion?
  if ( ctrlT ) {
    QStringList completions = s_completion->substringCompletion( s );
    if ( completions.count() > 1 ) {
      m_previousAddresses = prevAddr;
      setCompletedItems( completions, true );
    } else if ( completions.count() == 1 )
      setText( prevAddr + completions.first() );

    cursorAtEnd();
    return;
  }

  KGlobalSettings::Completion  mode = completionMode();

  switch ( mode ) {
    case KGlobalSettings::CompletionPopupAuto:
    {
      if ( s.isEmpty() )
        break;
    }

    case KGlobalSettings::CompletionPopup:
    {
      m_previousAddresses = prevAddr;
      QStringList items = s_completion->allMatches( s );
      items += s_completion->allMatches( "\"" + s );
      uint beforeDollarCompletionCount = items.count();

      if ( s.find( ' ' ) == -1 ) // one word, possibly given name
        items += s_completion->allMatches( "$$" + s );

      if ( !items.isEmpty() ) {
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
          int index = items.first().find( s );
          QString newText = prevAddr + items.first().mid( index );
          setUserSelection( false );
          setCompletedText( newText, true );
        }
      }

      break;
    }

    case KGlobalSettings::CompletionShell:
    {
      QString match = s_completion->makeCompletion( s );
      if ( !match.isNull() && match != s ) {
        setText( prevAddr + match );
        cursorAtEnd();
      }
      break;
    }

    case KGlobalSettings::CompletionMan: // Short-Auto in fact
    case KGlobalSettings::CompletionAuto:
    {
      if ( !s.isEmpty() ) {
        QString match = s_completion->makeCompletion( s );
        if ( !match.isNull() && match != s ) {
          QString adds = prevAddr + match;
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
  cursorAtEnd();
//  slotMatched( m_previousAddresses + completion );
}

void AddresseeLineEdit::loadContacts()
{
  s_completion->clear();
  s_addressesDirty = false;
  //m_contactMap.clear();

  KABC::Addressee::List list = contacts();
  KABC::Addressee::List::Iterator it;
  for ( it = list.begin(); it != list.end(); ++it )
    addContact( *it, 100 );
}

void AddresseeLineEdit::addContact( const KABC::Addressee& addr, int weight )
{
  //m_contactMap.insert( addr.realName(), addr );
  QString fullEmail = addr.fullEmail();
  s_completion->addItem( fullEmail.simplifyWhiteSpace(), weight );
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
    kdDebug(5300) << "LDAP completion entry: " << (*it).name << " " << (*it).email << " order=" << (*it).clientNumber << endl;
    KABC::Addressee addr;
    addr.setNameFromString( (*it).name );
    addr.insertEmail( (*it).email, true );
    // The weight is bigger for the first clients, lower for the last clients.
    // So weight = 50 - clientNumber;
    addContact( addr, 50 - (*it).clientNumber );
  }

  if ( hasFocus() || completionBox()->hasFocus() )
    if ( completionMode() != KGlobalSettings::CompletionNone )
      doCompletion( false );
}

KABC::Addressee::List AddresseeLineEdit::contacts()
{
  QApplication::setOverrideCursor( KCursor::waitCursor() ); // loading might take a while

  KABC::Addressee::List result;

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::AddressBook::Iterator it;
  for ( it = addressBook->begin(); it != addressBook->end(); ++it )
    result.append( *it );

  QApplication::restoreOverrideCursor();

  return result;
}

void AddresseeLineEdit::setCompletedItems( const QStringList& items, bool autoSuggest )
{
    QString txt = text();

    if ( !items.isEmpty() &&
         !(items.count() == 1 && txt == items.first()) )
    {
        if ( !txt.isEmpty() )
            completionBox()->setCancelledText( txt );

        completionBox()->setItems( items );
        completionBox()->popup();

        if ( autoSuggest )
        {
            int index = items.first().find( txt );
            QString newText = items.first().mid( index );
            setUserSelection(false);
            setCompletedText(newText,true);
        }
    }
    else
    {
        if ( completionBox() && completionBox()->isVisible() )
            completionBox()->hide();
    }
}

//static:
bool AddresseeLineEdit::getNameAndMail(const QString& aStr, QString& name, QString& mail)
{
  name = QString::null;
  mail = QString::null;

  const int len=aStr.length();

  bool bInComment;
  int i=0, iAd=0, iMailStart=0, iMailEnd=0;
  QChar c;

  // Find the '@' of the email address
  // skipping all '@' inside "(...)" comments:
  bInComment = false;
  while( i < len ){
    c = aStr[i];
    if( !bInComment ){
      if( '(' == c ){
        bInComment = true;
      }else{
        if( '@' == c ){
          iAd = i;
          break; // found it
        }
      }
    }else{
      if( ')' == c ){
        bInComment = false;
      }
    }
    ++i;
  }

  if( !iAd ){
    // We suppose the user is typing the string manually and just
    // has not finished typing the mail address part.
    // So we take everything that's left of the '<' as name and the rest as mail
    for( i = 0; len > i; ++i ) {
      c = aStr[i];
      if( '<' != c )
        name.append( c );
      else
        break;
    }
    mail = aStr.mid( i+1 );

  }else{

    // Loop backwards until we find the start of the string
    // or a ',' outside of a comment.
    bInComment = false;
    for( i = iAd-1; 0 <= i; --i ) {
      c = aStr[i];
      if( bInComment ){
        if( '(' == c ){
          if( !name.isEmpty() )
            name.prepend( ' ' );
          bInComment = false;
        }else{
          name.prepend( c ); // all comment stuff is part of the name
        }
      }else{
        // found the start of this addressee ?
        if( ',' == c )
          break;
        // stuff is before the leading '<' ?
        if( iMailStart ){
          name.prepend( c );
        }else{
          switch( c ){
            case '<':
              iMailStart = i;
              break;
            case ')':
              if( !name.isEmpty() )
                name.prepend( ' ' );
              bInComment = true;
              break;
            default:
              if( ' ' != c )
                mail.prepend( c );
          }
        }
      }
    }

    name = name.simplifyWhiteSpace();
    mail = mail.simplifyWhiteSpace();

    if( mail.isEmpty() )
      return false;

    mail.append('@');

    // Loop forward until we find the end of the string
    // or a ',' outside of a comment.
    bInComment = false;
    for( i = iAd+1; len > i; ++i ) {
      c = aStr[i];
      if( bInComment ){
        if( ')' == c ){
          if( !name.isEmpty() )
            name.append( ' ' );
          bInComment = false;
        }else{
          name.append( c ); // all comment stuff is part of the name
        }
      }else{
        // found the end of this addressee ?
        if( ',' == c )
          break;
        // stuff is behind the trailing '<' ?
        if( iMailEnd ){
          name.append( c );
        }else{
          switch( c ){
            case '>':
              iMailEnd = i;
              break;
            case '(':
              if( !name.isEmpty() )
                name.append( ' ' );
              bInComment = true;
              break;
            default:
              if( ' ' != c )
                mail.append( c );
          }
        }
      }
    }
  }

  name = name.simplifyWhiteSpace();
  mail = mail.simplifyWhiteSpace();

  return ! (name.isEmpty() || mail.isEmpty());
}

#include "addresseelineedit.moc"
