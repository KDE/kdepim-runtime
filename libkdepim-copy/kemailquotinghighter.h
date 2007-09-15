/**
 * kemailquotinghighter.h
 *
 * Copyright (C)  2006  Laurent Montel <montel@kde.org>
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
#ifndef LIBKDEPIM_KEMAILQUOTINGHIGHTER_H
#define LIBKDEPIM_KEMAILQUOTINGHIGHTER_H

#include <sonnet/highlighter.h>
#include <QSyntaxHighlighter>
#include <QStringList>

#include <kdepim_export.h>

class QTextEdit;
namespace KPIM
{
  class KDEPIM_EXPORT KEMailQuotingHighlighter : public Sonnet::Highlighter
    {
    public:
      enum SyntaxMode {
        PlainTextMode,
        RichTextMode
      };
      KEMailQuotingHighlighter( QTextEdit *textEdit,
                                      const QColor& QuoteColor0 = Qt::black,
                                      const QColor& QuoteColor1 = QColor( 0x00, 0x80, 0x00 ),
                                      const QColor& QuoteColor2 = QColor( 0x00, 0x80, 0x00 ),
                                      const QColor& QuoteColor3 = QColor( 0x00, 0x80, 0x00 ),
                                      SyntaxMode mode = PlainTextMode );
      ~KEMailQuotingHighlighter();
      
      void setSyntaxMode( SyntaxMode mode);

      void setQuoteColor( const QColor& QuoteColor0, const QColor& QuoteColor1, const QColor& QuoteColor2, const QColor& QuoteColor3);
            
      virtual void highlightBlock ( const QString & text );
	    
    private:
      class KEmailQuotingHighlighterPrivate;
      KEmailQuotingHighlighterPrivate *const d;
  
    };
}

#endif // LIBKDEPIM_KEMAILQUOTINGHIGHTER_H
