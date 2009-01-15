/**
 * kmeditor.cpp
 *
 * Copyright 2007 Laurent Montel <montel@kde.org>
 * Copyright 2008 Thomas McGuire <mcguire@kde.org>
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
#include "utils.h"
#include "maillistdrag.h"

#include <kpimidentities/signature.h>

#include <KCharsets>
#include <KComboBox>
#include <KCursor>
#include <KDirWatch>
#include <KEncodingFileDialog>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KPushButton>
#include <KProcess>
#include <KTemporaryFile>
#include <KToolBar>
#include <KWindowSystem>

#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QTextCodec>
#include <QAction>
#include <QProcess>
#include <QTimer>

#include <assert.h>

namespace KPIM {

class KMeditorPrivate
{
  public:
    KMeditorPrivate( KMeditor *parent )
     : q( parent ),
       useExtEditor( false ),
       mExtEditorProcess( 0 ),
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

    // Returns a list of positions of occurrences of the given signature.
    // Each pair is the index of the start- and end position of the signature.
    QList< QPair<int,int> > signaturePositions( const KPIMIdentities::Signature &sig ) const;

    void startExternalEditor();
    void slotEditorFinished( int, QProcess::ExitStatus exitStatus );

    // Data members
    QString extEditorPath;
    KMeditor *q;
    bool useExtEditor;

    // Although KTextEdit keeps track of the spell checking state, we override
    // it here, because we have a highlighter which does quote highlighting.
    // And since disabling spellchecking in KTextEdit simply would turn off our
    // quote highlighter, we never actually deactivate spell checking in the
    // base class, but only tell our own email highlighter to not highlight
    // spelling mistakes.
    // For this, we use the KTextEditSpellInterface, which is basically a hack
    // that makes it possible to have our own enabled/disabled state in a binary
    // compatible way.
    bool spellCheckingEnabled;

    KProcess *mExtEditorProcess;
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

      // Find the next occurrence of the signature text
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
    foreach( position, sigPositions ) { //krazy:exclude=foreach
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

void KMeditorPrivate::startExternalEditor()
{
  if ( extEditorPath.isEmpty() ) {
    q->setUseExternalEditor( false );
    //TODO: show messagebox
    return;
  }

  mExtEditorTempFile = new KTemporaryFile();
  if ( !mExtEditorTempFile->open() ) {
    delete mExtEditorTempFile;
    mExtEditorTempFile = 0;
    q->setUseExternalEditor( false );
    return;
  }

  mExtEditorTempFile->write( q->textOrHtml().toUtf8() );
  mExtEditorTempFile->flush();

  mExtEditorProcess = new KProcess();
    // construct command line...
  QStringList command = extEditorPath.split( ' ', QString::SkipEmptyParts );
  bool filenameAdded = false;
  for ( QStringList::Iterator it = command.begin(); it != command.end(); ++it ) {
    if ( ( *it ).contains( "%f" ) ) {
      ( *it ).replace( QRegExp( "%f" ), mExtEditorTempFile->fileName() );
      filenameAdded=true;
    }
    ( *mExtEditorProcess ) << ( *it );
  }
  if ( !filenameAdded )    // no %f in the editor command
    ( *mExtEditorProcess ) << mExtEditorTempFile->fileName();

  QObject::connect( mExtEditorProcess, SIGNAL( finished ( int, QProcess::ExitStatus ) ),
                    q, SLOT( slotEditorFinished( int, QProcess::ExitStatus ) ) );
  mExtEditorProcess->start();
  if ( !mExtEditorProcess->waitForStarted() ) {
    //TODO: messagebox
    mExtEditorProcess->deleteLater();
    mExtEditorProcess = 0;
    delete mExtEditorTempFile;
    mExtEditorTempFile = 0;
    q->setUseExternalEditor( false );
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

void KMeditorPrivate::slotEditorFinished(int, QProcess::ExitStatus exitStatus)
{
  if ( exitStatus == QProcess::NormalExit ) {
    mExtEditorTempFile->flush();
    mExtEditorTempFile->seek( 0 );
    QByteArray f = mExtEditorTempFile->readAll();
    q->setTextOrHtml( QString::fromUtf8( f.data(), f.size() ) );
  }

  q->killExternalEditor();   // cleanup...
}


void KMeditorPrivate::ensureCursorVisibleDelayed()
{
  static_cast<KRichTextWidget*>( q )->ensureCursorVisible();
}

void KMeditor::handleDragEvent( QDragMoveEvent *e )
{
  if ( KPIM::MailList::canDecode( e->mimeData() )
       || e->mimeData()->hasFormat( "image/png" ) )
  {
    e->accept();
    return;
  }

  // last chance: the source can provide no mimetype but urls (e.g. non KDE apps)
  KUrl::List urlList = KUrl::List::fromMimeData( e->mimeData() );
  if ( !urlList.isEmpty() )
    e->accept();
}

void KMeditor::dragEnterEvent( QDragEnterEvent *e )
{
  handleDragEvent( e );
  if ( e->isAccepted() )
    return;
  KRichTextWidget::dragEnterEvent( e );
}

void KMeditor::dragMoveEvent( QDragMoveEvent *e )
{
  handleDragEvent( e );
  if ( e->isAccepted() )
    return;
  KRichTextWidget::dragMoveEvent( e );
}

void KMeditor::insertFromMimeData( const QMimeData * source )
{
  // Attempt to paste HTML contents into the text edit in plain text mode,
  // prevent this and prevent plain text instead.
  if ( textMode() == KRichTextEdit::Plain && source->hasHtml() ) {
    if ( source->hasText() ) {
      insertPlainText( source->text() );
    }
  }
  else
    KRichTextWidget::insertFromMimeData( source );
}

void KMeditor::keyPressEvent ( QKeyEvent * e )
{
  if ( d->useExtEditor ) {
    if ( !d->mExtEditorProcess ) {
      d->startExternalEditor();
    }
    return;
  }

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
      KRichTextWidget::keyPressEvent( e );
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
      KRichTextWidget::keyPressEvent( e );
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
    KRichTextWidget::keyPressEvent( e );
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
  return KRichTextWidget::eventFilter( o, e );
}

void KMeditorPrivate::init()
{
  q->setSpellInterface(q);
  // We tell the KRichTextWidget to enable spell checking, because only then it will
  // call createHighlighter() which will create our own highlighter which also
  // does quote highlighting.
  // However, *our* spellchecking is still disabled. Our own highlighter only
  // cares about our spellcheck status, it will not highlight missspelled words
  // if our spellchecking is disabled.
  // See also KEMailQuotingHighlighter::highlightBlock().
  spellCheckingEnabled = false;
  q->setCheckSpellingEnabledInternal( true );

  KCursor::setAutoHideCursor( q, true, true );
  q->installEventFilter( q );
  QShortcut * insertMode = new QShortcut( QKeySequence( Qt::Key_Insert ), q );
  q->connect( insertMode, SIGNAL( activated() ),
              q, SLOT( slotChangeInsertMode() ) );
}

void KMeditor::slotChangeInsertMode()
{
  setOverwriteMode( !overwriteMode() );
  emit overwriteModeText();
}

bool KMeditor::isSpellCheckingEnabled() const
{
  return d->spellCheckingEnabled;
}

void KMeditor::setSpellCheckingEnabled( bool enable )
{
  KEMailQuotingHighlighter *hlighter =
      dynamic_cast<KEMailQuotingHighlighter*>( highlighter() );
  if ( hlighter )
    hlighter->toggleSpellHighlighting( enable );

  d->spellCheckingEnabled = enable;
  emit checkSpellingChanged( enable );
}

bool KMeditor::shouldBlockBeSpellChecked(const QString& block) const
{
  return quotePrefixName().simplified().isEmpty() ||
         !block.startsWith( quotePrefixName() );
}

void KMeditor::createHighlighter()
{
  KPIM::KEMailQuotingHighlighter *emailHighLighter =
      new KPIM::KEMailQuotingHighlighter( this );

  changeHighlighterColors( emailHighLighter );

  //TODO change config
  KRichTextWidget::setHighlighter( emailHighLighter );

  if ( !spellCheckingLanguage().isEmpty() )
    setSpellCheckingLanguage( spellCheckingLanguage() );
  setSpellCheckingEnabled( isSpellCheckingEnabled() );
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

KUrl KMeditor::insertFile()
{
  KEncodingFileDialog fdlg( QString(), QString(),  QString(), QString(),
                            KFileDialog::Opening, this );
  fdlg.okButton()->setText( i18n( "&Insert" ) );
  fdlg.setCaption( i18n( "Insert File" ) );
  if ( !fdlg.exec() )
    return KUrl();
  else {
    KUrl url = fdlg.selectedUrl();
    url.setFileEncoding( fdlg.selectedEncoding() );
    return url;
  }
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
  if ( d->mExtEditorProcess )
    d->mExtEditorProcess->deleteLater();
  d->mExtEditorProcess = 0;
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
    insertPlainText( Utils::rot13( cursor.selectedText() ) );
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
    bool hackForCursorsAtEnd = false;
    int oldCursorPos = -1;
    if ( placement == End ) {

      if ( oldCursor.position() == toPlainText().length() ) {
        hackForCursorsAtEnd = true;
        oldCursorPos = oldCursor.position();
      }

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

    // There is one special case when re-setting the old cursor: The cursor
    // was at the end. In this case, QTextEdit has no way to know
    // if the signature was added before or after the cursor, and just decides
    // that it was added before (and the cursor moves to the end, but it should
    // not when appending a signature). See bug 167961
    if ( hackForCursorsAtEnd )
      oldCursor.setPosition( oldCursorPos );

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

    // Find the next occurrence of the signature text
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

  // Get rid of embedded linebreaks, caused by the <br> tag. We add our own,
  // plain-text ascii linebreaks above.
  // Additionally, when using plain text mode, the text doesn't contain the
  // line separator character, so we can't even rely on that.
  temp.remove( QChar::LineSeparator );

  // Get rid of embedded images, see QTextImageFormat documentation:
  // "Inline images are represented by an object replacement character (0xFFFC in Unicode) "
  temp.remove( 0xFFFC );

  return temp;
}

QString KMeditor::toCleanPlainText() const
{
  QString temp = toPlainText();
  temp.remove( 0xFFFC ); // See toWrappedPlainText().
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
