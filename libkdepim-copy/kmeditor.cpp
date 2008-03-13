/**
 * kmeditor.cpp
 *
 * Copyright (C)  2007 Laurent Montel <montel@kde.org>
 * Copyright (C)  2008 Thomas McGuire <thomas.mcguire@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "kmeditor.h"
#include "kemailquotinghighter.h"

#include "kmutils.h"
#include <maillistdrag.h>

//kdepimlibs includes
#include <kpimidentities/signature.h>

//kdelibs includes
#include <kdebug.h>
#include <kdeversion.h>
#include <kfind.h>
#include <kreplace.h>
#include <kreplacedialog.h>
#include <kfinddialog.h>
#include <KWindowSystem>
#include <KFindDialog>
#include <KColorDialog>
#include <KFileDialog>
#include <KComboBox>
#include <KToolBar>
#include <KCharsets>
#include <KPushButton>
#include <klocale.h>
#include <kmenu.h>
#include <KMessageBox>
#include <KDirWatch>
#include <KTemporaryFile>
#include <KCursor>
#include <sonnet/configdialog.h><n

//qt includes
#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QPointer>
#include <QTextCodec>
#include <QTextList>
#include <QAction>
#include <QProcess>
#include <QTextLayout>

//system includes
#include <assert.h>

namespace KPIM {

class KMeditorPrivate
{
  public:
    KMeditorPrivate( KMeditor *parent )
     : q( parent ),
       useExtEditor( false ),
       mExtEditorProcess( 0 ),
       mExtEditorTempFileWatcher( 0 ),
       mExtEditorTempFile( 0 ),
       mMode( KMeditor::Plain )
    {
    }

    ~KMeditorPrivate()
    {
    }

    //
    // Slots
    //
    void addSuggestion( const QString&,const QStringList& );


    //
    // Normal functions
    //
    QString addQuotesToText( const QString &inputText );
    QString removeQuotesFromText( const QString &inputText ) const;
    void init();

    // Switches to rich text mode and emits the mode changed signal if the
    // mode really changed.
    void activateRichText();

    // Returns the text of the signature. If the signature is HTML, the HTML
    // tags will be stripped.
    QString plainSignatureText( const KPIMIdentities::Signature &signature ) const;

    // Replaces each text which matches the regular expression with another text.
    // Text inside quotes or the given signature will be ignored.
    void cleanWhitespaceHelper( const QRegExp &regExp, const QString &newText,
                                const KPIMIdentities::Signature &sig );

    // Returns a list of positions of occurences of the given signature.
    // Each pair is the index of the start- and end position of the signature.
    QList< QPair<int,int> > signaturePositions( const KPIMIdentities::Signature &sig ) const;

    // Data members
    QMap<QString,QStringList> replacements;
    QString extEditorPath;
    QString language;
    KMeditor *q;
    bool useExtEditor;
    bool spellCheckingEnabled;
    QProcess *mExtEditorProcess;
    KDirWatch *mExtEditorTempFileWatcher;
    KTemporaryFile *mExtEditorTempFile;
    KMeditor::Mode mMode;
};

}

using namespace KPIM;

void KMeditorPrivate::activateRichText()
{
  if ( mMode == KMeditor::Plain ) {
    q->setAcceptRichText( true );
    mMode = KMeditor::Rich;
    emit q->textModeChanged( mMode );
  }
}

QList< QPair<int,int> > KMeditorPrivate::signaturePositions( const KPIMIdentities::Signature &sig ) const
{
  QList< QPair<int,int> > signaturePositions;
  if ( !sig.rawText().isEmpty() ) {

    QString sigText = plainSignatureText( sig );

    int currentSearchPosition = 0;
    forever {

      // Find the next occurence of the signature text
      QString text = q->document()->toPlainText();
      int currentMatch = text.indexOf( sigText, currentSearchPosition );
      currentSearchPosition = currentMatch + sigText.length();
      if ( currentMatch == -1 )
        break;

      signaturePositions.append( QPair<int,int>( currentMatch,
                                                 currentMatch + sigText.length() ) );
    }
  }
  return signaturePositions;
}

void KMeditorPrivate::cleanWhitespaceHelper( const QRegExp &regExp,
                                             const QString &newText,
                                             const KPIMIdentities::Signature &sig )
{
  int currentSearchPosition = 0;

  forever {

    // Find the text
    QString text = q->document()->toPlainText();
    int currentMatch = regExp.indexIn( text, currentSearchPosition );
    currentSearchPosition = currentMatch;
    if ( currentMatch == -1 )
      break;

    // Select the text
    QTextCursor cursor( q->document() );
    cursor.setPosition( currentMatch );
    cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                         regExp.matchedLength() );

    // Skip quoted text
    if ( cursor.block().text().startsWith( q->quotePrefixName() ) ) {
      currentSearchPosition += regExp.matchedLength();
      continue;
    }

    // Skip text inside signatures
    bool insideSignature = false;
    QList< QPair<int,int> > sigPositions = signaturePositions( sig );
    QPair<int,int> position;
    foreach( position, sigPositions ) {
      if ( cursor.position() >= position.first &&
           cursor.position() <= position.second )
        insideSignature = true;
    }
    if ( insideSignature ) {
      currentSearchPosition += regExp.matchedLength();
      continue;
    }

    // Replace the text
    cursor.removeSelectedText();
    cursor.insertText( newText );
    currentSearchPosition += newText.length();
  }
}

QString KMeditorPrivate::plainSignatureText( const KPIMIdentities::Signature &signature ) const
{
  QString sigText = signature.rawText();
  if ( signature.isInlinedHtml() &&
       signature.type() == KPIMIdentities::Signature::Inlined ) {

    // Use a QTextDocument as a helper, it does all the work for us and
    // strips all HTML tags.
    QTextDocument helper;
    QTextCursor helperCursor( &helper );
    helperCursor.insertHtml( sigText );
    sigText = helper.toPlainText();
  }
  return sigText;
}

void KMeditorPrivate::addSuggestion( const QString& originalWord,
                                     const QStringList& lst )
{
  replacements[originalWord] = lst;
}

void KMeditor::dragEnterEvent( QDragEnterEvent *e )
{
  if ( KPIM::MailList::canDecode( e->mimeData() ) ) {
    e->setAccepted( true );
  } else if ( e->mimeData()->hasFormat( "image/png" ) ) {
    e->accept();
  } else {
    return KTextEdit::dragEnterEvent( e );
  }
}

void KMeditor::dragMoveEvent( QDragMoveEvent *e )
{
  if ( KPIM::MailList::canDecode( e->mimeData() ) ) {
    e->accept();
  } else if  ( e->mimeData()->hasFormat( "image/png" ) ) {
    e->accept();
  } else {
    return KTextEdit::dragMoveEvent( e );
  }
}

void KMeditor::paste()
{
  if ( !QApplication::clipboard()->image().isNull() )  {
    emit pasteImage();
  }
  else
    KTextEdit::paste();
}

void KMeditor::keyPressEvent ( QKeyEvent * e )
{
  if ( e->key() ==  Qt::Key_Return ) {
    QTextCursor cursor = textCursor();
    int oldPos = cursor.position();
    int blockPos = cursor.block().position();

    //selection all the line.
    cursor.movePosition( QTextCursor::StartOfBlock );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
    QString lineText = cursor.selectedText();
    if ( ( ( oldPos -blockPos )  > 0 ) &&
          ( ( oldPos-blockPos ) < int( lineText.length() ) ) ) {
      bool isQuotedLine = false;
      int bot = 0; // bot = begin of text after quote indicators
      while ( bot < lineText.length() ) {
        if( ( lineText[bot] == '>' ) || ( lineText[bot] == '|' ) ) {
          isQuotedLine = true;
          ++bot;
        }
        else if ( lineText[bot].isSpace() ) {
          ++bot;
        }
        else {
          break;
        }
      }
      KTextEdit::keyPressEvent( e );
      // duplicate quote indicators of the previous line before the new
      // line if the line actually contained text (apart from the quote
      // indicators) and the cursor is behind the quote indicators
      if ( isQuotedLine
         && ( bot != lineText.length() )
         && ( ( oldPos-blockPos ) >= int( bot ) ) ) {
        // The cursor position might have changed unpredictably if there was selected
        // text which got replaced by a new line, so we query it again:
        cursor.movePosition( QTextCursor::StartOfBlock );
        cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
        QString newLine = cursor.selectedText();

        // remove leading white space from the new line and instead
        // add the quote indicators of the previous line
        int leadingWhiteSpaceCount = 0;
        while ( ( leadingWhiteSpaceCount < newLine.length() )
               && newLine[leadingWhiteSpaceCount].isSpace() ) {
          ++leadingWhiteSpaceCount;
        }
        newLine = newLine.replace( 0, leadingWhiteSpaceCount,
                                   lineText.left( bot ) );
        cursor.insertText( newLine );
        //cursor.setPosition( cursor.position() + 2);
        cursor.movePosition( QTextCursor::StartOfBlock );
        setTextCursor( cursor );
      }
    }
    else
      KTextEdit::keyPressEvent( e );
  }
  else if ( e->key() == Qt::Key_Up && e->modifiers() != Qt::ShiftModifier
            && textCursor().blockNumber()== 0 /*First block*/
            && textCursor().block().position() == 0 )
  {
    textCursor().clearSelection();
    emit focusUp();
  }
  else if ( e->key() == Qt::Key_Backtab && e->modifiers() == Qt::ShiftModifier )
  {
    textCursor().clearSelection();
    emit focusUp();
  }
  else
  {
    KTextEdit::keyPressEvent( e );
  }
}

KMeditor::KMeditor( const QString& text, QWidget *parent )
 : KTextEdit( text, parent ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::KMeditor( QWidget *parent )
 : KTextEdit( parent ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::~KMeditor()
{
  delete d;
}

bool KMeditor::eventFilter( QObject*o, QEvent* e )
{
  if (o == this)
    KCursor::autoHideEventFilter( o, e );
  return KTextEdit::eventFilter( o, e );
}

void KMeditorPrivate::init()
{
  // We tell the KTextEdit to enable spell checking, because only then it will
  // call createHighlighter() which will create our own highlighter which also
  // does quote highlighting.
  // However, *our* spellchecking is still disabled. Our own highlighter only
  // cares about our spellcheck status, it will not highlight missspellt words
  // if our spellchecking is disabled.
  // See also KEMailQuotingHighlighter::highlightBlock().
  spellCheckingEnabled = false;
  static_cast<KTextEdit*>( q )->setCheckSpellingEnabled( true );

  KCursor::setAutoHideCursor( q, true, true );
  q->installEventFilter( q );
  QShortcut * insertMode = new QShortcut( QKeySequence( Qt::Key_Insert ), q );
  q->connect( insertMode, SIGNAL( activated() ),
              q, SLOT( slotChangeInsertMode() ) );
#if KDE_IS_VERSION(4,0,67)
  q->connect( q, SIGNAL( languageChanged(const QString &) ),
              q, SLOT( setSpellCheckLanguage(const QString &) ) );
#endif
}

void KMeditor::slotChangeInsertMode()
{
  setOverwriteMode( !overwriteMode() );
  emit overwriteModeText();
}

void KMeditor::createHighlighter()
{
  KPIM::KEMailQuotingHighlighter *emailHighLighter =
      new KPIM::KEMailQuotingHighlighter( this );

  changeHighlighterColors( emailHighLighter );

  connect( emailHighLighter, SIGNAL( newSuggestions(const QString&,const QStringList&) ),
           this, SLOT( addSuggestion(const QString&,const QStringList&) ) );

  //TODO change config
  KTextEdit::setHighlighter( emailHighLighter );

  if ( !d->language.isEmpty() )
    setSpellCheckLanguage( d->language );
  toggleSpellChecking( d->spellCheckingEnabled );
}

void KMeditor::changeHighlighterColors(KPIM::KEMailQuotingHighlighter *)
{
}

void KMeditor::setUseExternalEditor( bool use )
{
  d->useExtEditor = use;
}

void KMeditor::setExternalEditorPath( const QString & path )
{
  d->extEditorPath = path;
}


void KMeditor::slotChangeParagStyle( QTextListFormat::Style _style )
{
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();

  // Create a list with the specified format
  if ( _style != QTextListFormat::ListStyleUndefined ) {

    QTextBlockFormat blockFmt = cursor.blockFormat();
    QTextListFormat listFmt;

    if ( cursor.currentList() ) {
      listFmt = cursor.currentList()->format();
    } else {
      listFmt.setIndent( blockFmt.indent() + 1 );
      blockFmt.setIndent( 0 );
      cursor.setBlockFormat( blockFmt );
    }

    listFmt.setStyle( _style );

    cursor.createList( listFmt );
    d->activateRichText();
  }

  // Remove the list formatting again
  else {
    QTextList *list = cursor.currentList();
    if ( list ) {

      QTextListFormat listFormat = list->format();
      listFormat.setIndent( 0 );
      list->setFormat( listFormat );

      int count = list->count();
      while ( count > 0 ) {
        list->removeItem( 0 );
        count--;
      }
    }
  }

  cursor.endEditBlock();
  setFocus();
}

void KMeditor::setColor( const QColor& col )
{
  QTextCharFormat fmt;
  fmt.setForeground( col );
  textCursor().mergeCharFormat( fmt );
  d->activateRichText();
}

void KMeditor::setFont( const QFont &font )
{
  QTextCharFormat fmt;
  fmt.setFont( font );
  textCursor().mergeCharFormat( fmt );
}

void KMeditor::setFontForWholeText( const QFont &font )
{
  QTextCharFormat fmt;
  fmt.setFont( font );
  QTextCursor cursor( document() );
  cursor.movePosition( QTextCursor::End, QTextCursor::KeepAnchor );
  cursor.mergeCharFormat( fmt );
  document()->setDefaultFont( font );
}

void KMeditor::slotAlignLeft()
{
  setAlignment( Qt::AlignLeft );
  d->activateRichText();
}

void KMeditor::slotAlignCenter()
{
  setAlignment( Qt::AlignHCenter );
  d->activateRichText();
}

void KMeditor::slotAlignRight()
{
  setAlignment( Qt::AlignRight );
  d->activateRichText();
}

void KMeditor::slotTextBold( bool _b )
{
  QTextCharFormat fmt;
  fmt.setFontWeight( _b ? QFont::Bold : QFont::Normal );
  textCursor().mergeCharFormat( fmt );
  d->activateRichText();
}

void KMeditor::slotTextItalic( bool _b)
{
  QTextCharFormat fmt;
  fmt.setFontItalic( _b );
  textCursor().mergeCharFormat( fmt );
  d->activateRichText();
}

void KMeditor::slotTextUnder( bool _b )
{
  QTextCharFormat fmt;
  fmt.setFontUnderline( _b );
  textCursor().mergeCharFormat( fmt );
  d->activateRichText();
}

void KMeditor::slotTextColor()
{
  QColor color = textColor();

  if ( KColorDialog::getColor( color, this ) ) {
    QTextCharFormat fmt;
    fmt.setForeground( color );
    textCursor().mergeCharFormat( fmt );
    d->activateRichText();
  }
}

void KMeditor::slotFontFamilyChanged( const QString &f )
{
  QTextCharFormat fmt;
  fmt.setFontFamily( f );
  textCursor().mergeCharFormat( fmt );
  setFocus();
  d->activateRichText();
}

void KMeditor::slotFontSizeChanged( int size )
{
  QTextCharFormat fmt;
  fmt.setFontPointSize( size );
  textCursor().mergeCharFormat( fmt );
  setFocus();
  d->activateRichText();
}

KUrl KMeditor::insertFile( const QStringList &encodingLst, QString &encodingStr )
{
  KUrl url;
  KFileDialog fdlg( url, QString(), this );
  fdlg.setOperationMode( KFileDialog::Opening );
  fdlg.okButton()->setText( i18n( "&Insert" ) );
  fdlg.setCaption( i18n( "Insert File" ) );
  if ( !encodingLst.isEmpty() )
  {
    KComboBox *combo = new KComboBox( this );
    combo->addItems( encodingLst );
    fdlg.toolBar()->addWidget( combo );
    for ( int i = 0; i < combo->count(); i++ )
      if ( KGlobal::charsets()->codecForName( KGlobal::charsets()->
                                       encodingForName( combo->itemText( i ) ) )
           == QTextCodec::codecForLocale() )
        combo->setCurrentIndex(i);
    encodingStr = combo->currentText();
  }
  if ( !fdlg.exec() )
    return KUrl();

  KUrl u = fdlg.selectedUrl();
  return u;
}

void KMeditor::wordWrapToggled( bool on )
{
  if ( on ) {
    setLineWrapMode( QTextEdit::FixedColumnWidth );
  } else {
    setLineWrapMode (QTextEdit::NoWrap );
  }
}

void KMeditor::contextMenuEvent( QContextMenuEvent *event )
{
  QTextCursor cursor = textCursor();
  if ( cursor.hasSelection() )
  {
    //if we have selection so use standard menu
    KTextEdit::contextMenuEvent( event );
    return;
  }
  else
  {
    //select word under current cursor
    cursor.select( QTextCursor::WordUnderCursor );
    setTextCursor( cursor );
    QString word = textCursor().selectedText();
    if ( word.isEmpty() || !d->replacements.contains( word ) )
      KTextEdit::contextMenuEvent( event );
    else //try to create spell check menu
    {
      KMenu p;
      p.addTitle( i18n( "Suggestions" ) );

      //Add the suggestions to the popup menu
      QStringList reps = d->replacements[word];
      if ( reps.count() > 0 ) {
        for ( QStringList::Iterator it = reps.begin(); it != reps.end(); ++it ) {
          p.addAction( *it );
        }
      }
      else {
        p.addAction( i18n( "No Suggestions" ) );
      }

      //Execute the popup inline
      const QAction *selectedAction = p.exec( mapToGlobal( event->pos() ) );

      if ( selectedAction && ( reps.count() > 0 ) ) {
        int oldPos = cursor.position();
        const QString replacement = selectedAction->text();
        cursor.insertText( replacement );

#if 0
        // Restore the cursor position; if the cursor was behind the
        // misspelled word then adjust the cursor position
        if ( para == parIdx && txtIdx >= lastSpace )
          txtIdx += replacement.length() - word.length();
          setCursorPosition( parIdx, txtIdx );
#endif
        cursor.setPosition(oldPos);
        setTextCursor(cursor);
      }
      return;
    }
  }
}

void KMeditor::slotPasteAsQuotation()
{
  if ( hasFocus() ) {
    QString s = QApplication::clipboard()->text();
    if ( !s.isEmpty() ) {
      insertPlainText( d->addQuotesToText( s ) );
    }
  }
}

void KMeditor::slotRemoveQuotes()
{
  // TODO: I think this is backwards.
  // i.e, if no region is marked then remove quotes from every line
  // else remove quotes only on the lines that are marked.
  QTextCursor cursor = textCursor();
  if(cursor.hasSelection())
  {
    QString s = cursor.selectedText();
    insertPlainText( d->removeQuotesFromText( s ) );
  }
  else
  {
    int oldPos = cursor.position();
    cursor.movePosition( QTextCursor::StartOfBlock );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
    QString s = cursor.selectedText();
    if ( !s.isEmpty() )
    {
      cursor.insertText( d->removeQuotesFromText( s ) );
      cursor.setPosition( qMax( 0, oldPos - 2 ) );
      setTextCursor( cursor );
    }
  }
}

void KMeditor::slotAddQuotes()
{
  // TODO: I think this is backwards.
  // i.e, if no region is marked then add quotes to every line
  // else add quotes only on the lines that are marked.
  QTextCursor cursor = textCursor();
  if ( cursor.hasSelection() )
  {
    QString s = cursor.selectedText();
    if ( !s.isEmpty() ) {
      cursor.insertText( d->addQuotesToText( s ) );
      setTextCursor( cursor );
    }
  }
  else {
    int oldPos = cursor.position();
    cursor.movePosition( QTextCursor::StartOfBlock );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
    QString s = cursor.selectedText();
    cursor.insertText( d->addQuotesToText( s ) );
    cursor.setPosition( oldPos + 2 ); //FIXME: 2 probably wrong
    setTextCursor( cursor );
  }
}

QString KMeditorPrivate::removeQuotesFromText( const QString &inputText ) const
{
  QString s = inputText;

  // remove first leading quote
  QString quotePrefix = '^' + q->quotePrefixName();
  QRegExp rx( quotePrefix );
  s.remove( rx );

  // now remove all remaining leading quotes
  quotePrefix = QString( QChar::ParagraphSeparator ) + ' ' + q->quotePrefixName();
  QRegExp srx( quotePrefix );
  s.remove( srx );

  return s;
}

QString KMeditorPrivate::addQuotesToText( const QString &inputText )
{
  QString answer = QString( inputText );
  QString indentStr = q->quotePrefixName();
  answer.replace( '\n', '\n' + indentStr );
  //cursor.selectText() as QChar::ParagraphSeparator as paragraph separator.
  answer.replace( QChar::ParagraphSeparator, '\n' + indentStr );
  answer.prepend( indentStr );
  answer += '\n';
  return q->smartQuote( answer );
}

QString KMeditor::smartQuote( const QString & msg )
{
  return msg;
}

QString KMeditor::quotePrefixName() const
{
  //Need to redefine into each apps
  return "> ";
}

bool KMeditor::checkExternalEditorFinished()
{
  if ( !d->mExtEditorProcess )
    return true;
  switch ( KMessageBox::warningYesNoCancel( topLevelWidget(),
           i18n("The external editor is still running.\n"
                "Abort the external editor or leave it open?"),
           i18n("External Editor"),
           KGuiItem( i18n("Abort Editor") ),
           KGuiItem( i18n("Leave Editor Open") ) ) ) {
  case KMessageBox::Yes:
    killExternalEditor();
    return true;
  case KMessageBox::No:
    return true;
  default:
    return false;
  }
}

void KMeditor::killExternalEditor()
{
  delete d->mExtEditorProcess;
  d->mExtEditorProcess = 0;
  delete d->mExtEditorTempFileWatcher;
  d->mExtEditorTempFileWatcher = 0;
  delete d->mExtEditorTempFile;
  d->mExtEditorTempFile = 0;
}


void KMeditor::setCursorPositionFromStart( unsigned int pos )
{
  if ( pos > 0 )
  {
    QTextCursor cursor = textCursor();
    cursor.setPosition( pos );
    setTextCursor( cursor );
    ensureCursorVisible();
  }
}

void KMeditor::slotRemoveBox()
{
  //Laurent: fix me
#if 0
    if (hasMarkedText()) {
    QString s = QString::fromLatin1("\n") + markedText() + QString::fromLatin1("\n");
    s.replace(QRegExp("\n,----[^\n]*\n"),"\n");
    s.replace(QRegExp("\n| "),"\n");
    s.replace(QRegExp("\n`----[^\n]*\n"),"\n");
    s.remove(0,1);
    s.truncate(s.length()-1);
    insert(s);
  } else {
    int l = currentLine();
    int c = currentColumn();

    QString s = textLine(l);   // test if we are in a box
    if (!((s.left(2) == "| ")||(s.left(5)==",----")||(s.left(5)=="`----")))
      return;

    setAutoUpdate(false);

    // find & remove box begin
    int x = l;
    while ((x>=0)&&(textLine(x).left(5)!=",----"))
      x--;
    if ((x>=0)&&(textLine(x).left(5)==",----")) {
      removeLine(x);
      l--;
      for (int i=x;i<=l;i++) {     // remove quotation
        s = textLine(i);
        if (s.left(2) == "| ") {
          s.remove(0,2);
          insertLine(s,i);
          removeLine(i+1);
        }
      }
    }

    // find & remove box end
    x = l;
    while ((x<numLines())&&(textLine(x).left(5)!="`----"))
      x++;
    if ((x<numLines())&&(textLine(x).left(5)=="`----")) {
      removeLine(x);
      for (int i=l+1;i<x;i++) {     // remove quotation
        s = textLine(i);
        if (s.left(2) == "| ") {
          s.remove(0,2);
          insertLine(s,i);
          removeLine(i+1);
        }
      }
    }

    setCursorPosition(l,c-2);

    setAutoUpdate(true);
    repaint();
  }
#endif
}

void KMeditor::slotAddBox()
{
  //Laurent fixme
  QTextCursor cursor = textCursor();
  if ( cursor.hasSelection() )
  {
    QString s = cursor.selectedText();
    s.prepend( ",----[  ]\n" );
    s.replace( QRegExp("\n"),"\n| " );
    s.append( "\n`----" );
    insertPlainText( s );
  } else {
    //int oldPos = cursor.position();
    cursor.movePosition( QTextCursor::StartOfBlock );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
    QString s = cursor.selectedText();
    QString str = QString::fromLatin1(",----[  ]\n| %1\n`----").arg(s);
    cursor.insertText( str );
    //cursor.setPosition( qMax( 0, oldPos - 2 ) );
    setTextCursor( cursor );
  }
}

int KMeditor::linePosition()
{
  QTextCursor cursor = textCursor();
  return cursor.blockNumber();
}

int KMeditor::columnNumber()
{
  QTextCursor cursor = textCursor();
  return cursor.columnNumber();
}

void KMeditor::slotRot13()
{
  QTextCursor cursor = textCursor();
  if ( cursor.hasSelection() )
    insertPlainText( KMUtils::rot13( cursor.selectedText() ) );
  //FIXME: breaks HTML formatting
}

void KMeditor::setCursorPosition( int linePos, int columnPos )
{
  //TODO
  Q_UNUSED( linePos );
  Q_UNUSED( columnPos );
}

void KMeditor::cleanWhitespace( const KPIMIdentities::Signature &sig )
{
  QTextCursor cursor( document() );
  cursor.beginEditBlock();

  // Squeeze tabs and spaces
  d->cleanWhitespaceHelper( QRegExp( "[\t ]+" ), QString( ' ' ), sig );

  // Remove trailing whitespace
  d->cleanWhitespaceHelper( QRegExp( "[\t ][\n]" ), QString( '\n' ), sig );

  // Single space lines
  d->cleanWhitespaceHelper( QRegExp( "[\n]{3,}" ), "\n\n", sig );

  if ( !textCursor().hasSelection() ) {
    textCursor().clearSelection();
  }

  cursor.endEditBlock();
}

void KMeditor::insertSignature( const KPIMIdentities::Signature &sig,
                                Placement placement, bool addSeparator )
{
  QString signature;
  if ( addSeparator )
    signature = sig.withSeparator();
  else
    signature = sig.rawText();

  insertSignature( signature, placement,
                   ( sig.isInlinedHtml() &&
                     sig.type() == KPIMIdentities::Signature::Inlined ) );
}

void KMeditor::insertSignature( const QString &signature, Placement placement,
                                bool isHtml )
{
  if ( !signature.isEmpty() ) {

    // Save the modified state of the document, as inserting a signature
    // shouldn't change this. Restore it at the end of this function.
    bool isModified = document()->isModified();

    // Move to the desired position, where the signature should be inserted
    QTextCursor cursor = textCursor();
    QTextCursor oldCursor = cursor;
    cursor.beginEditBlock();

    if ( placement == End )
      cursor.movePosition( QTextCursor::End );
    else if ( placement == Start )
      cursor.movePosition( QTextCursor::Start );
    setTextCursor( cursor );

    // Insert the signature and newlines depending on where it was inserted.
    if ( placement == End ) {
      if ( isHtml ) {
        insertHtml( "<br>" + signature );
      } else {
        insertPlainText( '\n' + signature );
      }
    } else if ( placement == Start || placement == AtCursor ) {
      if ( isHtml ) {
        insertHtml( "<br>" + signature + "<br>" );
      } else {
        insertPlainText( '\n' + signature + '\n' );
      }
    }

    cursor.endEditBlock();
    setTextCursor( oldCursor );
    ensureCursorVisible();

    document()->setModified( isModified );

    if ( isHtml )
      d->activateRichText();
  }
}

void KMeditor::replaceSignature( const KPIMIdentities::Signature &oldSig,
                                 const KPIMIdentities::Signature &newSig )
{
  QTextCursor cursor( document() );
  cursor.beginEditBlock();

  QString oldSigText = d->plainSignatureText( oldSig );

  int currentSearchPosition = 0;
  forever {

    // Find the next occurence of the signature text
    QString text = document()->toPlainText();
    int currentMatch = text.indexOf( oldSigText, currentSearchPosition );
    currentSearchPosition = currentMatch;
    if ( currentMatch == -1 )
      break;

    // Select the signature
    QTextCursor cursor( document() );
    cursor.setPosition( currentMatch );

    // If the new signature is completely empty, we also want to remove the
    // signature separator, so include it in the selection
    int additionalMove = 0;
    if ( newSig.rawText().isEmpty() &&
         text.mid( currentMatch - 4, 4) == "-- \n" ) {
      cursor.movePosition( QTextCursor::PreviousCharacter,
                           QTextCursor::MoveAnchor, 4 );
      additionalMove = 4;
    }
    cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                         oldSigText.length() + additionalMove );


    // Skip quoted signatures
    if ( cursor.block().text().startsWith( quotePrefixName() ) ) {
      currentSearchPosition += d->plainSignatureText( oldSig ).length();
      continue;
    }

    // Remove the old and instert the new signature
    cursor.removeSelectedText();
    if ( newSig.isInlinedHtml() &&
         newSig.type() == KPIMIdentities::Signature::Inlined ) {
      cursor.insertHtml( newSig.rawText() );
      d->activateRichText();
    }
    else
      cursor.insertText( newSig.rawText() );

    currentSearchPosition += d->plainSignatureText( newSig ).length();
  }

  cursor.endEditBlock();
}

void KMeditor::switchToPlainText()
{
  if ( d->mMode == Rich ) {
    d->mMode = Plain;
    // TODO: Warn the user about this?
    document()->setPlainText( document()->toPlainText() );
    setAcceptRichText( false );
    emit textModeChanged( d->mMode );
  }
}

KMeditor::Mode KMeditor::textMode() const
{
  return d->mMode;
}

QString KMeditor::textOrHTML() const
{
  if ( textMode() == Rich )
    return toHtml();
  else
    return toPlainText();
}

void KMeditor::focusOutEvent( QFocusEvent *e )
{
  KTextEdit::focusOutEvent( e );
  emit focusChanged( false );
}

void KMeditor::focusInEvent( QFocusEvent *e )
{
  KTextEdit::focusInEvent( e );
  emit focusChanged( true );
}

void KMeditor::setSpellCheckLanguage( const QString &language )
{
  if ( KTextEdit::highlighter() ) {
    d->replacements.clear();
    KTextEdit::highlighter()->setCurrentLanguage( language );
    KTextEdit::highlighter()->rehighlight();
  }
#if KDE_IS_VERSION(4,0,60)
  KTextEdit::setSpellCheckingLanguage( language );
#endif

  if ( language != d->language )
    emit spellcheckLanguageChanged( language );
  d->language = language;
}

void KMeditor::slotCheckSpelling()
{
  KTextEdit::checkSpelling();
}

bool KMeditor::isSpellCheckingEnabled() const
{
  return d->spellCheckingEnabled;
}

void KMeditor::toggleSpellChecking( bool on )
{
  KEMailQuotingHighlighter *hlighter =
            dynamic_cast<KEMailQuotingHighlighter*>( KTextEdit::highlighter() );
  if ( hlighter )
    hlighter->toggleSpellHighlighting( on );
  if ( !on )
    d->replacements.clear();
  d->spellCheckingEnabled = on;
}

void KMeditor::showSpellConfigDialog( const QString &configFileName )
{
  KConfig config( configFileName );
  Sonnet::ConfigDialog dialog( &config, this );
#if KDE_IS_VERSION(4,0,67)
  if ( !d->language.isEmpty() )
    dialog.setLanguage( d->language );
  connect( &dialog, SIGNAL( languageChanged(const QString &) ),
           this, SLOT( setSpellCheckLanguage(const QString &) ) );
#endif
  dialog.setWindowIcon( KIcon( "internet-mail" ) );
  dialog.exec();
}

#include "kmeditor.moc"
