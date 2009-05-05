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
#include "utils.h"
#include "maillistdrag.h"

#include <kpimidentities/signature.h>

#include <KCharsets>
#include <KComboBox>
#include <KCursor>
#include <KDebug>
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

using namespace KPIMTextEdit;

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

    void init();
    QString addQuotesToText( const QString &inputText );

    void startExternalEditor();
    void slotEditorFinished( int, QProcess::ExitStatus exitStatus );

    /**
     * Replaces each text which matches the regular expression with another text.
     * Text inside quotes or the given signature will be ignored.
     */
    void cleanWhitespaceHelper( const QRegExp &regExp, const QString &newText,
                                const KPIMIdentities::Signature &sig );

    /**
     * Returns a list of all occurences of the given signature.
     * The list contains pairs which consists of the starting position and the end
     * of the signature.
     *
     * @param sig this signature will be searched for
     * @return a list of pairs of start and end positions of the signature
     */
    QList< QPair<int,int> > signaturePositions( const KPIMIdentities::Signature &sig ) const;

    /**
     * Returns the text of the signature. If the signature is HTML, the HTML
     * tags will be stripped.
     */
    QString plainSignatureText( const KPIMIdentities::Signature &signature ) const;

    // Data members
    QString extEditorPath;
    KMeditor *q;
    bool useExtEditor;

    KProcess *mExtEditorProcess;
    KTemporaryFile *mExtEditorTempFile;
};

}

using namespace KPIM;

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
  static_cast<KPIMTextEdit::TextEdit*>( q )->ensureCursorVisible();
}

void KMeditor::keyPressEvent ( QKeyEvent * e )
{
  if ( d->useExtEditor ) {
    if ( !d->mExtEditorProcess ) {
      d->startExternalEditor();
    }
    return;
  }

  if ( e->key() == Qt::Key_Up && e->modifiers() != Qt::ShiftModifier &&
       textCursor().block().position() == 0 &&
       textCursor().position() < textCursor().block().layout()->lineAt( 0 ).textLength() )
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
    TextEdit::keyPressEvent( e );
  }
}

KMeditor::KMeditor( const QString& text, QWidget *parent )
 : TextEdit( text, parent ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::KMeditor( QWidget *parent )
 : TextEdit( parent ), d( new KMeditorPrivate( this ) )
{
  d->init();
}

KMeditor::~KMeditor()
{
  delete d;
}

void KMeditorPrivate::init()
{
  QShortcut * insertMode = new QShortcut( QKeySequence( Qt::Key_Insert ), q );
  q->connect( insertMode, SIGNAL( activated() ),
              q, SLOT( slotChangeInsertMode() ) );
}

void KMeditor::slotChangeInsertMode()
{
  setOverwriteMode( !overwriteMode() );
  emit insertModeChanged();
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
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  if ( !cursor.hasSelection() )
    cursor.select( QTextCursor::Document );

  QTextBlock block = document()->findBlock( cursor.selectionStart() );
  int selectionEnd = cursor.selectionEnd();
  while ( block.isValid() && block.position() <= selectionEnd ) {
    cursor.setPosition( block.position() );
    if ( isLineQuoted( block.text() ) ) {
      int length = quoteLength( block.text() );
      cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, length );
      cursor.removeSelectedText();
      selectionEnd -= length;
    }
    block = block.next();
  }
  cursor.clearSelection();
  cursor.endEditBlock();
}

void KMeditor::slotAddQuotes()
{
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  if ( !cursor.hasSelection() )
    cursor.select( QTextCursor::Document );

  QTextBlock block = document()->findBlock( cursor.selectionStart() );
  int selectionEnd = cursor.selectionEnd();
  while ( block.isValid() && block.position() <= selectionEnd ) {
    cursor.setPosition( block.position() );
    cursor.insertText( defaultQuoteSign() );
    selectionEnd += defaultQuoteSign().length();
    block = block.next();
  }
  cursor.clearSelection();
  cursor.endEditBlock();
}

QString KMeditorPrivate::addQuotesToText( const QString &inputText )
{
  QString answer = QString( inputText );
  QString indentStr = q->defaultQuoteSign();
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
  const QTextCursor cursor = textCursor();
  const QTextDocument* doc = document();
  QTextBlock block = doc->begin();
  int lineCount = 0;

  // Simply using cursor.block.blockNumber() would not work since that does not
  // take word-wrapping into account, i.e. it is possible to have more than one line
  // in a block.
  //
  // What we have to do therefore is to iterate over the blocks and count the lines
  // in them. Once we have reached the block where the cursor is, we have to iterate
  // over each line in it, to find the exact line in the block where the cursor is.
  while ( block.isValid() ) {
    const QTextLayout *layout = block.layout();

    // If the current block has the cursor in it, iterate over all its lines
    if ( block == cursor.block() ) {

      // Special case: Cursor at end of single non-wrapped line, exit early
      // in this case as the logic below can't handle it
      if ( block.lineCount() == layout->lineCount() )
        return lineCount;

      const int cursorBasePosition = cursor.position() - block.position();
      for ( int i = 0; i < layout->lineCount(); i++ ) {
        QTextLine line = layout->lineAt( i );
        if ( cursorBasePosition >= line.textStart() &&
             cursorBasePosition < line.textStart() + line.textLength() )
          break;
        lineCount++;
      }
      return lineCount;
    }

    // No, cursor is not in the current block
    else
      lineCount += layout->lineCount();

    block = block.next();
  }

  // Only gets here if the cursor block can't be found, shouldn't happen except
  // for an empty document maybe
  return lineCount;
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
    if ( q->isLineQuoted( cursor.block().text() ) ) {
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

void KMeditor::cleanWhitespace( const KPIMIdentities::Signature &sig )
{
  QTextCursor cursor( document() );
  cursor.beginEditBlock();

  // Squeeze tabs and spaces
  d->cleanWhitespaceHelper( QRegExp( QLatin1String( "[\t ]+" ) ),
                            QString( QLatin1Char( ' ' ) ), sig );

  // Remove trailing whitespace
  d->cleanWhitespaceHelper( QRegExp( QLatin1String( "[\t ][\n]" ) ),
                            QString( QLatin1Char( '\n' ) ), sig );

  // Single space lines
  d->cleanWhitespaceHelper( QRegExp( QLatin1String( "[\n]{3,}" ) ),
                            QLatin1String( "\n\n" ), sig );

  if ( !textCursor().hasSelection() ) {
    textCursor().clearSelection();
  }

  cursor.endEditBlock();
}

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


void KMeditor::replaceSignature( const KPIMIdentities::Signature &oldSig,
                                 const KPIMIdentities::Signature &newSig )
{
  QTextCursor cursor( document() );
  cursor.beginEditBlock();

  QString oldSigText = d->plainSignatureText( oldSig );

  int currentSearchPosition = 0;
  forever {

    // Find the next occurrence of the signature text
    QString text = toPlainText();
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
         text.mid( currentMatch - 4, 4 ) == QLatin1String( "-- \n" ) ) {
      cursor.movePosition( QTextCursor::PreviousCharacter,
                           QTextCursor::MoveAnchor, 4 );
      additionalMove = 4;
    }
    cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                         oldSigText.length() + additionalMove );


    // Skip quoted signatures
    if ( isLineQuoted( cursor.block().text() ) ) {
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

#include "kmeditor.moc"
