/**
 * kmeditor.cpp
 *
 * Copyright (C)  2007 Laurent Montel <montel@kde.org>
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

#include <maillistdrag.h>

//kdelibs includes
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

//qt includes
#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QPointer>
#include <QTextList>
#include <QAction>

class KMeditor::Private
{
  public:
    Private(KTextEdit *_parent)
     : parent(_parent),useExtEditor(false),findDialog(0L),find(0L)
    {
    }

    ~Private()
    {
      delete find;
      delete replace;
    }

    void addSuggestion(const QString&,const QStringList&);

    void slotHighlight( const QString &, int, int );

    QMap<QString,QStringList> replacements;

    QString extEditorPath;
    KTextEdit *parent;
    bool useExtEditor;
    QPointer<KFindDialog> findDialog;
    QPointer<KReplaceDialog> replaceDialog;
    KFind *find;
    KReplace *replace;
};

void KMeditor::Private::slotHighlight( const QString &word, int start, int end)
{
  kDebug()<<" KMeditor::Private::slotHighlight( const QString &, int, int ) :"<<word<< " pos start :"<<start<<" pos end :"<<end<<endl;
  parent->highlightWord( end, start );
}

void KMeditor::Private::addSuggestion(const QString& originalWord,const QStringList& lst)
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

void KMeditor::dropEvent( QDropEvent *e )
{
  //Need to reimplement it by each apps
  KTextEdit::dropEvent(e);
}

void KMeditor::paste()
{
  if ( ! QApplication::clipboard()->image().isNull() )  {
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
       cursor.movePosition(QTextCursor::StartOfBlock);
       cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
       QString lineText = cursor.selectedText();
       if ( ( ( oldPos -blockPos )  > 0 ) && ( ( oldPos-blockPos ) < int( lineText.length() ) ) ) {
       bool isQuotedLine = false;
       int bot = 0; // bot = begin of text after quote indicators
       while( bot < lineText.length() ) {
         if( ( lineText[bot] == '>' ) || ( lineText[bot] == '|' ) ) {
           isQuotedLine = true;
           ++bot;
         }
         else if( lineText[bot].isSpace() ) {
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
       if( isQuotedLine
          && ( bot != lineText.length() )
          && ( ( oldPos-blockPos ) >= int( bot ) ) ) {


        // The cursor position might have changed unpredictably if there was selected
        // text which got replaced by a new line, so we query it again:
        int newPos = cursor.position();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString newLine = cursor.selectedText();

        // remove leading white space from the new line and instead
        // add the quote indicators of the previous line
        int leadingWhiteSpaceCount = 0;
        while( ( leadingWhiteSpaceCount < newLine.length() )
               && newLine[leadingWhiteSpaceCount].isSpace() ) {
          ++leadingWhiteSpaceCount;
        }
        newLine = newLine.replace( 0, leadingWhiteSpaceCount,
                                   lineText.left( bot ) );
        cursor.insertText(newLine);
        //cursor.setPosition( cursor.position() + 2);
        setTextCursor(cursor);
      }
    }
    else
      KTextEdit::keyPressEvent( e );
  }
  else
    KTextEdit::keyPressEvent( e );
}

KMeditor::KMeditor( const QString& text, QWidget *parent)
 : KTextEdit(text, parent), d( new Private(this) )
{
   init();
}

KMeditor::KMeditor( QWidget *parent)
 : KTextEdit(parent), d( new Private(this) )
{
  init();
}

KMeditor::~KMeditor()
{
  delete d;
}

void KMeditor::init()
{
   //TODO change it.
   new QShortcut( Qt::CTRL+Qt::Key_F, this, SLOT(slotFindNext()) );

   //TODO change it.
   new QShortcut( Qt::CTRL+Qt::Key_R, this, SLOT(slotReplaceNext()) );
   //enable spell checking by default
   setCheckSpellingEnabled(true);
}

void KMeditor::createHighlighter()
{
   Sonnet::KEMailQuotingHighlighter *emailHighLighter = new Sonnet::KEMailQuotingHighlighter(this);
   connect(emailHighLighter,SIGNAL(newSuggestions(const QString&,const QStringList&)),this, SLOT(addSuggestion(const QString&,const QStringList&)) );

   //TODO change config
   setHightighter(emailHighLighter);
}


void KMeditor::setUseExternalEditor( bool use )
{
   d->useExtEditor = use;
}

void KMeditor::setExternalEditorPath( const QString & path )
{
  d->extEditorPath = path;
}

void KMeditor::findText()
{
  // Raise if already opened
  if ( d->findDialog )
  {
#ifdef Q_WS_X11
    KWindowSystem::activateWindow( d->findDialog->winId() );
#else
    d->findDialog->activateWindow();
#endif
    return;
  }
  d->findDialog = new KFindDialog( this );
  d->findDialog->setAttribute(Qt::WA_DeleteOnClose);
  d->findDialog->setHasSelection( !textCursor().selectedText().isEmpty() );
  //Display dialogbox
  d->findDialog->show();
  connect( d->findDialog, SIGNAL(okClicked()), this, SLOT(slotFindNext()) );

  findText( d->findDialog->pattern(), 0 /*options*/, this, d->findDialog );
}


void KMeditor::slotReplaceNext()
{
  //TODO
}


void KMeditor::slotFindNext()
{
  findTextNext();
}

void KMeditor::findTextNext()
{
  if (!d->find)
  {
    findText();
    return;
  }
  long options = 0;
  if(d->findDialog)
  {
    if ( d->find->pattern() != d->findDialog->pattern() )
    {
      d->find->setPattern( d->findDialog->pattern() );
      d->find->resetCounts();
    }
    options = d->findDialog->options();
  }
  KFind::Result res = KFind::NoMatch;
  while( res == KFind::NoMatch )
  {
      if ( d->find->needData() )
      {
        d->find->setData(toPlainText());
        res = d->find->find();
      }
      res = d->find->find();
  }
  if ( res == KFind::NoMatch ) // i.e. we're done
  {
      if ( d->find->shouldRestart() )
      {
        findTextNext();
      }
      else // really done
      {
       //initFindNode( false, options & KFind::FindBackwards, false );
        d->find->resetCounts();
        //slotClearSelection();
	d->find->closeFindNextDialog();
      }
  }
}

void KMeditor::findText( const QString &str, long options, QWidget *parent, KFindDialog *findDialog )
{
  if(toPlainText().isEmpty())
    return;

  delete d->find;
  kDebug()<<" str :"<<str<<endl;
  //configure it
  d->find = new KFind(str, options,parent,findDialog);
  d->find->closeFindNextDialog();
  connect(d->find,SIGNAL( highlight( const QString &, int, int ) ),
          this, SLOT( slotHighlight( const QString &, int, int ) ) );

}

void KMeditor::replaceText()
{
  //TODO
}

void KMeditor::slotChangeParagStyle(QTextListFormat::Style _style)
{
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();

  QTextBlockFormat blockFmt = cursor.blockFormat();

  QTextListFormat listFmt;

  if (cursor.currentList()) {
     listFmt = cursor.currentList()->format();
  } else {
     listFmt.setIndent(blockFmt.indent() + 1);
     blockFmt.setIndent(0);
     cursor.setBlockFormat(blockFmt);
  }

  listFmt.setStyle(_style);

  cursor.createList(listFmt);

  cursor.endEditBlock();
}

void KMeditor::slotAlignLeft()
{
  setAlignment(Qt::AlignLeft);
}

void KMeditor::slotAlignCenter()
{
  setAlignment(Qt::AlignHCenter);
}

void KMeditor::slotAlignRight()
{
  setAlignment(Qt::AlignRight);
}

void KMeditor::slotTextBold( bool _b )
{
  QTextCharFormat fmt;
  fmt.setFontWeight(_b ? QFont::Bold : QFont::Normal);
  mergeFormat(fmt);
}

void KMeditor::slotTextItalic( bool _b)
{
  QTextCharFormat fmt;
  fmt.setFontItalic(_b);
  mergeFormat(fmt);
}

void KMeditor::slotTextUnder( bool _b)
{
  QTextCharFormat fmt;
  fmt.setFontUnderline(_b);
  mergeFormat(fmt);
}

void KMeditor::mergeFormat(const QTextCharFormat &format)
{
    QTextCursor cursor = textCursor();
    cursor.mergeCharFormat(format);
    mergeCurrentCharFormat(format);
}

void KMeditor::slotTextColor()
{
  QColor color = textColor();

  if ( KColorDialog::getColor( color, this ) ) {
    QTextCharFormat fmt;
    fmt.setForeground(color);
    mergeFormat(fmt);
  }
}

void KMeditor::slotFontFamilyChanged(const QString &f)
{
   QTextCharFormat fmt;
   fmt.setFontFamily(f);
   mergeFormat(fmt);
}

void KMeditor::slotFontSizeChanged(int size)
{
  QTextCharFormat fmt;
  fmt.setFontPointSize(size);
  mergeFormat(fmt);
}

void KMeditor::switchTextMode(bool useHtml)
{
  //TODO: need to look at what change which highlighter.

  if(useHtml && acceptRichText()) //Already in HTML mode
     return;
  if(!useHtml && !acceptRichText()) //Already in text mode
     return;

  if(useHtml)
  {
    setAcceptRichText(true);
    document()->setModified( true );
  }
  else
  {
    setAcceptRichText(false);
    setText(toPlainText()); //reformat text (which can be html text) as text
    document()->setModified(true );
  }
}

KUrl KMeditor::insertFile(const QStringList & encodingLst)
{
  KUrl url;
  KFileDialog fdlg( url, QString(), this );
  fdlg.setOperationMode( KFileDialog::Opening );
  fdlg.okButton()->setText(i18n("&Insert"));
  fdlg.setCaption(i18n("Insert File"));
  if(!encodingLst.isEmpty())
  {
    KComboBox *combo = new KComboBox( this );
    combo->addItems( encodingLst );
    fdlg.toolBar()->addWidget( combo );
    for (int i = 0; i < combo->count(); i++)
      if (KGlobal::charsets()->codecForName(KGlobal::charsets()->
                                          encodingForName(combo->itemText(i)))
        == QTextCodec::codecForLocale()) combo->setCurrentIndex(i);
  }
  if (!fdlg.exec()) return KUrl();

  KUrl u = fdlg.selectedUrl();
  return u;
}

void KMeditor::wordWrapToggled( bool on, bool wrapWidth )
{
  if ( on ) {
    if(wrapWidth)
    {
	//FIXME
    	//setLineWrapMode(QTextEdit::FixedColumnWidth & QTextEdit::WidgetWidth);
    }
    else
	setLineWrapMode(QTextEdit::FixedColumnWidth);
  } else {
    setLineWrapMode (QTextEdit::NoWrap );
  }
}

void KMeditor::contextMenuEvent( QContextMenuEvent *event )
{
  QTextCursor cursor = textCursor();
  if(cursor.hasSelection())
  {
    //if we have selection so use standard menu
    KTextEdit::contextMenuEvent( event );
    return;
  }
  else
  {
    //select word under current cursor
    cursor.select(QTextCursor::WordUnderCursor);
    setTextCursor(cursor);
    QString word = textCursor().selectedText();
    if(word.isEmpty() || !d->replacements.contains( word ) )
       KTextEdit::contextMenuEvent( event );
    else //try to create spell check menu
    {
        KMenu p;
        p.addTitle( i18n("Suggestions") );

        //Add the suggestions to the popup menu
        QStringList reps = d->replacements[word];
        if ( reps.count() > 0 ) {
          for ( QStringList::Iterator it = reps.begin(); it != reps.end(); ++it ) {
            p.addAction( *it );
          }
        }
        else {
          p.addAction( i18n("No Suggestions") );
        }

        //Execute the popup inline
        const QAction *selectedAction = p.exec( mapToGlobal( event->pos() ) );

        if ( selectedAction && ( reps.count() > 0 ) ) {
	  int oldPos=cursor.position();
          const QString replacement = selectedAction->text();
	  cursor.insertText(replacement);

#if 0
          // Restore the cursor position; if the cursor was behind the
          // misspelled word then adjust the cursor position
          if ( para == parIdx && txtIdx >= lastSpace )
            txtIdx += replacement.length() - word.length();
          setCursorPosition(parIdx, txtIdx);
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
  QString s = QApplication::clipboard()->text();
  if ( !s.isEmpty() ) {
    insert( addQuotesToText( s ) );
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
     insert( removeQuotesFromText( s ) );
   }
   else
   {
     int oldPos = cursor.position();
     cursor.movePosition(QTextCursor::StartOfBlock);
     cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
     QString s = cursor.selectedText();
     cursor.insertText(removeQuotesFromText( s ));
     cursor.setPosition( oldPos -2 );
     setTextCursor(cursor);
   }
}

QString KMeditor::removeQuotesFromText( const QString &inputText ) const
{
  QString s = inputText;

  // remove first leading quote
  QString quotePrefix = '^' + quotePrefixName();
  QRegExp rx( quotePrefix );
  s.remove( rx );

  // now remove all remaining leading quotes
  quotePrefix = '\n' + quotePrefixName();
  QRegExp srx( quotePrefix );
  s.replace( srx, "\n" );

  return s;
}

QString KMeditor::addQuotesToText( const QString &inputText )
{
  QString answer = QString( inputText );
  QString indentStr = quotePrefixName();
  answer.replace( '\n', '\n' + indentStr );
  answer.prepend( indentStr );
  answer += '\n';
  return smartQuote( answer );
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

#include "kmeditor.moc"
