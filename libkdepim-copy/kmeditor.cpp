/**
 * kemeditor.cpp
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


class KMeditor::Private
{
  public:
    Private()
     : useExtEditor(false)
    {
    }

    ~Private()
    {
    }

    void addSuggestion(const QString&,const QStringList&);

    QMap<QString,QStringList> replacements;   
    
    QString extEditorPath;
    bool useExtEditor;
};

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

void KMeditor::keyPressEvent ( QKeyEvent * e )
{
#if 0
 if( e->key() == Qt::Key_Return ) {
    int line, col;
    getCursorPosition( &line, &col );
    QString lineText = text( line );
    // returns line with additional trailing space (bug in Qt?), cut it off
    lineText.truncate( lineText.length() - 1 );
    // special treatment of quoted lines only if the cursor is neither at
    // the begin nor at the end of the line
    if( ( col > 0 ) && ( col < int( lineText.length() ) ) ) {
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

      KEdit::keyPressEvent( e );

      // duplicate quote indicators of the previous line before the new
      // line if the line actually contained text (apart from the quote
      // indicators) and the cursor is behind the quote indicators
      if( isQuotedLine
          && ( bot != lineText.length() )
          && ( col >= int( bot ) ) ) {

        // The cursor position might have changed unpredictably if there was selected
        // text which got replaced by a new line, so we query it again:
        getCursorPosition( &line, &col );
        QString newLine = text( line );
        // remove leading white space from the new line and instead
        // add the quote indicators of the previous line
        int leadingWhiteSpaceCount = 0;
        while( ( leadingWhiteSpaceCount < newLine.length() )
               && newLine[leadingWhiteSpaceCount].isSpace() ) {
          ++leadingWhiteSpaceCount;
        }
        newLine = newLine.replace( 0, leadingWhiteSpaceCount,
                                   lineText.left( bot ) );
        removeParagraph( line );
        insertParagraph( newLine, line );
        // place the cursor at the begin of the new line since
        // we assume that the user split the quoted line in order
        // to add a comment to the first part of the quoted line
        setCursorPosition( line, 0 );
      }
    }
    else
      KTextEdit::keyPressEvent( e );
  }
  else
    KTextEdit::keyPressEvent( e );
#endif

  KTextEdit::keyPressEvent(e);
}

KMeditor::KMeditor( const QString& text, QWidget *parent)
 : KTextEdit(text, parent), d( new Private() )
{
   init();
}

KMeditor::KMeditor( QWidget *parent)
 : KTextEdit(parent), d( new Private() )
{
  init();
}

KMeditor::~KMeditor()
{
  delete d;
}

void KMeditor::init()
{
   //enable spell checking by default
   setCheckSpellingEnabled(true);
}

void KMeditor::createHighlighter()
{
   KSpell2::KEMailQuotingHighlighter *emailHighLighter = new KSpell2::KEMailQuotingHighlighter(this);
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


#include "kmeditor.moc"
