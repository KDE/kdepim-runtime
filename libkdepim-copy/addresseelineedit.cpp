/*
    This file is part of KAddressBook.
    Copyright (c) 2002 Helge Deller <deller@gmx.de>
                  2002 Lubos Lunak <llunak@suse.cz>
                  2001,2003 Carsten Pfeiffer <pfeiffer@kde.org>
                  2001 Waldo Bastian <bastian@kde.org>

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
#include <kabc/ldapclient.h>
#include <kabc/stdaddressbook.h>

#include "addresseelineedit.h"

using namespace KPIM;

KCompletion * AddresseeLineEdit::s_completion = 0L;
bool AddresseeLineEdit::s_addressesDirty = false;
QTimer* AddresseeLineEdit::s_LDAPTimer = 0L;
KABC::LdapSearch* AddresseeLineEdit::s_LDAPSearch = 0L;
QString* AddresseeLineEdit::s_LDAPText = 0L;
AddresseeLineEdit* AddresseeLineEdit::s_LDAPLineEdit = 0L;

static KStaticDeleter<KCompletion> completionDeleter;
static KStaticDeleter<QTimer> ldapTimerDeleter;
static KStaticDeleter<KABC::LdapSearch> ldapSearchDeleter;
static KStaticDeleter<QString> ldapTextDeleter;

AddresseeLineEdit::AddresseeLineEdit( QWidget* parent, bool useCompletion,
                                      const char *name )
  : KLineEdit( parent, name )
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
    s_completion->setOrder( KCompletion::Sorted );
    s_completion->setIgnoreCase( true );
  }

  if ( m_useCompletion ) {
    if ( !s_LDAPTimer ) {
      ldapTimerDeleter.setObject( s_LDAPTimer, new QTimer );
      ldapSearchDeleter.setObject( s_LDAPSearch, new KABC::LdapSearch );
      ldapTextDeleter.setObject( s_LDAPText, new QString );
    }
    connect( s_LDAPTimer, SIGNAL( timeout() ), SLOT( slotStartLDAPLookup() ) );
    connect( s_LDAPSearch, SIGNAL( searchData( const QStringList& ) ),
             SLOT( slotLDAPSearchData( const QStringList& ) ) );
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
/*
void AddresseeLineEdit::mouseReleaseEvent( QMouseEvent * e )
{
  if ( m_useCompletion && (e->button() == MidButton) ) {
    m_smartPaste = true;
    KLineEdit::mouseReleaseEvent(e);
    m_smartPaste = false;
    return;
  }

  KLineEdit::mouseReleaseEvent( e );
}
*/
void AddresseeLineEdit::insert( const QString &t )
{
  if ( !m_smartPaste ) {
    KLineEdit::insert( t );
    return;
  }

  QString newText = t.stripWhiteSpace();
  if ( newText.isEmpty() )
    return;

  // remove newlines in the to-be-pasted string as well as an eventual
  // mailto: protocol
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
    loadAddresses();

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
      items += s_completion->substringCompletion( '<' + s );
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
}

void AddresseeLineEdit::loadAddresses()
{
  s_completion->clear();
  s_addressesDirty = false;

  QStringList adrs = addresses();
  for ( QStringList::ConstIterator it = adrs.begin(); it != adrs.end(); ++it)
    addAddress( *it );
}

void AddresseeLineEdit::addAddress( const QString& adr )
{
  s_completion->addItem( adr );
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

  if ( s.length() == 0 )
    return;

  loadAddresses(); // TODO reuse these?
  s_LDAPSearch->startSearch( s );
}

void AddresseeLineEdit::slotLDAPSearchData( const QStringList& adrs )
{
  if ( s_LDAPLineEdit != this )
    return;

  for ( QStringList::ConstIterator it = adrs.begin(); it != adrs.end(); ++it ) {
    int pos = (*it).find( '<' );

    if ( pos == -1 )
      addAddress( *it );
    else {
      QString name = (*it).left( pos );
      if ( name.find( '@' ) == -1 )
        addAddress( name );
    }
  }

  if ( hasFocus() || completionBox()->hasFocus() )
    if ( completionMode() != KGlobalSettings::CompletionNone )
      doCompletion( false );
}

QStringList AddresseeLineEdit::addresses()
{
  QApplication::setOverrideCursor( KCursor::waitCursor() ); // loading might take a while

  QStringList result;

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::AddressBook::Iterator it;
  for ( it = addressBook->begin(); it != addressBook->end(); ++it )
    result.append( (*it).realName() );

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

#include "addresseelineedit.moc"
