/**
 * kmeditor.cpp
 *
 * Copyright 2007 Laurent Montel <montel@kde.org>
 * Copyright 2008 Thomas McGuire <thomas.mcguire@gmx.net>
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
#include <kdeversion.h>
#include <KWindowSystem>
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
#include <sonnet/configdialog.h>

//qt includes
#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QTextCodec>
#include <QAction>
#include <QProcess>
#include <QTimer>

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
       mExtEditorTempFile( 0 )    {
    }

    ~KMeditorPrivate()
    {
    }

    //
    // Slots
    //

    // Just calls KTextEdit::ensureCursorVisible(), workaround for some bug.
    void ensureCursorVisibleDelayed();

    //
    // Normal functions
    //

    QString addQuotesToText( const QString &inputText );
    QString removeQuotesFromText( const QString &inputText ) const;
    void init();

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
    QString extEditorPath;
    QString language;
    KMeditor *q;
    bool useExtEditor;
    bool spellCheckingEnabled;
    QProcess *mExtEditorProcess;
    KDirWatch *mExtEditorTempFileWatcher;
    KTemporaryFile *mExtEditorTempFile;
};

}

using namespace KPIM;

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

void KMeditorPrivate::ensureCursorVisibleDelayed()
{
  static_cast<KTextEdit*>( q )->ensureCursorVisible();
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
  else if ( e->key() == Qt::Key_Up && e->modifiers() != Qt::ShiftModifier &&
            textCursor().block().position() == 0 &&
            textCursor().position() <
                        textCursor().block().layout()->lineAt( 0 ).textLength() )
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
 : KRichTextWidget( text, parent ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::KMeditor( QWidget *parent )
 : KRichTextWidget( parent ), d( new KMeditorPrivate( this ) )
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
  // cares about our spellcheck status, it will not highlight missspelled words
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

void KMeditor::setFontForWholeText( const QFont &font )
{
  QTextCharFormat fmt;
  fmt.setFont( font );
  QTextCursor cursor( document() );
  cursor.movePosition( QTextCursor::End, QTextCursor::KeepAnchor );
  cursor.mergeCharFormat( fmt );
  document()->setDefaultFont( font );
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

void KMeditor::enableWordWrap( int wrapColumn )
{
  setWordWrapMode( QTextOption::WordWrap );
  setLineWrapMode( QTextEdit::FixedColumnWidth );
  setLineWrapColumnOrWidth( wrapColumn );
}

void KMeditor::disableWordWrap()
{
  setLineWrapMode( QTextEdit::WidgetWidth );
}

void KMeditor::contextMenuEvent( QContextMenuEvent *event )
{
#if KDE_IS_VERSION(4,0,73)
  // Obtain the cursor at the mouse position and the current cursor
  QTextCursor cursorAtMouse = cursorForPosition( event->pos() );
  int mousePos = cursorAtMouse.position();
  QTextCursor cursor = textCursor();

  // Check if the user clicked a selected word
  bool selectedWordClicked = cursor.hasSelection() &&
                             mousePos >= cursor.selectionStart() &&
                             mousePos <= cursor.selectionEnd();

  // Get the word under the (mouse-)cursor and see if it is misspelled.
  // Don't include apostrophes at the start/end of the word in the selection.
  QTextCursor wordSelectCursor( cursorAtMouse );
  wordSelectCursor.clearSelection();
  wordSelectCursor.select( QTextCursor::WordUnderCursor );
  QString selectedWord = wordSelectCursor.selectedText();

  // Clear the selection again, we re-select it below (without the apostrophes).
  wordSelectCursor.movePosition( QTextCursor::PreviousCharacter,
                                 QTextCursor::MoveAnchor, selectedWord.size() );
  if ( selectedWord.startsWith( '\'' ) || selectedWord.startsWith( '\"' ) ) {
    selectedWord = selectedWord.right( selectedWord.size() - 1 );
    wordSelectCursor.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor );
  }
  if ( selectedWord.endsWith( '\'' ) || selectedWord.endsWith( '\"' ))
    selectedWord.chop( 1 );
  wordSelectCursor.movePosition( QTextCursor::NextCharacter,
                                 QTextCursor::KeepAnchor, selectedWord.size() );

  bool wordIsMisspelled = !selectedWord.isEmpty() &&
                          KTextEdit::highlighter()->isWordMisspelled( selectedWord );

  // If the user clicked a selected word, do nothing.
  // If the user clicked somewhere else, move the cursor there.
  // If the user clicked on a misspelled word, select that word.
  // Same behavior as in OpenOffice Writer.
  bool inQuote = cursorAtMouse.block().text().startsWith( quotePrefixName() );
  if ( !selectedWordClicked ) {
    if ( wordIsMisspelled && !inQuote )
      setTextCursor( wordSelectCursor );
    else
      setTextCursor( cursorAtMouse );
    cursor = textCursor();
  }

  // Use standard context menu for already selected words, correctly spelled
  // words and words inside quotes.
  if ( !wordIsMisspelled || selectedWordClicked || inQuote ) {
    KTextEdit::contextMenuEvent( event );
  }
  else {
    KMenu suggestions;
    KMenu menu;

    //Add the suggestions to the popup menu
    QStringList reps = KTextEdit::highlighter()->suggestionsForWord( selectedWord );
    if ( reps.count() > 0 ) {
      for ( QStringList::Iterator it = reps.begin(); it != reps.end(); ++it ) {
        suggestions.addAction( *it );
      }

    }
    suggestions.setTitle( i18n( "Suggestions" ) );

    QAction *ignoreAction = menu.addAction( i18n("Ignore") );
    QAction *addToDictAction = menu.addAction( i18n("Add to Dictionary") );
    QAction *suggestionsAction = menu.addMenu( &suggestions );
    if ( reps.count() == 0 )
      suggestionsAction->setEnabled( false );

    //Execute the popup inline
    const QAction *selectedAction = menu.exec( mapToGlobal( event->pos() ) );

    if ( selectedAction ) {
      Q_ASSERT( cursor.selectedText() == selectedWord );

      if ( selectedAction == ignoreAction ) {
        KTextEdit::highlighter()->ignoreWord( selectedWord );
        KTextEdit::highlighter()->rehighlight();
      }
      else if ( selectedAction == addToDictAction ) {
        KTextEdit::highlighter()->addWordToDictionary( selectedWord );
        KTextEdit::highlighter()->rehighlight();
      }

      // Other actions can only be one of the suggested words
      else {
        const QString replacement = selectedAction->text();
        Q_ASSERT( reps.contains( replacement ) );
        cursor.insertText( replacement );
        setTextCursor( cursor );
      }
    }
  }
#else
  KTextEdit::contextMenuEvent( event );
  kWarning() << "spell check menu not compiled in, kdelibs too old.";
#endif
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
      enableRichTextMode();
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
      enableRichTextMode();
    }
    else
      cursor.insertText( newSig.rawText() );

    currentSearchPosition += d->plainSignatureText( newSig ).length();
  }

  cursor.endEditBlock();
}

void KMeditor::setSpellCheckLanguage( const QString &language )
{
  if ( KTextEdit::highlighter() ) {
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

QString KMeditor::toWrappedPlainText() const
{
  QString temp;
  QTextDocument* doc = document();
  QTextBlock block = doc->begin();
  while ( block.isValid() ) {
    QTextLayout* layout = block.layout();
    for ( int i = 0; i < layout->lineCount(); i++ ) {
      QTextLine line = layout->lineAt( i );
      temp += block.text().mid( line.textStart(), line.textLength() ) + '\n';
    }
    block = block.next();
  }
  return temp;
}

void KMeditor::ensureCursorVisible()
{
  QCoreApplication::processEvents();

  // Hack: In KMail, the layout of the composer changes again after
  //       creating the editor (the toolbar/menubar creation is delayed), so
  //       the size of the editor changes as well, possibly hiding the cursor
  //       even though we called ensureCursorVisible() before the layout phase.
  //
  //       Delay the actual call to ensureCursorVisible() a bit to work around
  //       the problem.
  QTimer::singleShot( 500, this, SLOT( ensureCursorVisibleDelayed() ) );
}

#include "kmeditor.moc"
