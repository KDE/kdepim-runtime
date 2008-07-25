/**
 * kemailquotinghighter.h
 *
 * Copyright (C)  2006  Laurent Montel <montel@kde.org>
 * Copyright (C)  2008  Thomas McGuire <thomas.mcguire@gmx.net>
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
#ifndef KDEPIM_KEMAILQUOTINGHIGHTER_H
#define KDEPIM_KEMAILQUOTINGHIGHTER_H

#include "kdepim_export.h"

#include <sonnet/highlighter.h>

class QTextEdit;

namespace KPIM
{
  /**
   * This highlighter highlights spelling mistakes and also highlightes
   * quotes.
   *
   * Spelling mistakes inside quotes will not be highlighted.
   * The quote highlighting color is configurable.
   *
   * Spell highlighting is disabled by default but can be toggled.
   */
  class KDEPIM_EXPORT KEMailQuotingHighlighter : public Sonnet::Highlighter
    {
    public:

      // FIXME: Default colors don't obey color scheme
      // The normalColor parameter will be ignored, only provided for KNode
      // compatibility.
      explicit KEMailQuotingHighlighter( QTextEdit *textEdit,
                                         const QColor &normalColor = Qt::black,
                                         const QColor &quoteDepth1 = QColor( 0x00, 0x80, 0x00 ),
                                         const QColor &quoteDepth2 = QColor( 0x00, 0x80, 0x00 ),
                                         const QColor &quoteDepth3 = QColor( 0x00, 0x80, 0x00 ),
                                         const QColor &misspelledColor = Qt::red );

      ~KEMailQuotingHighlighter();

      // The normalColor parameter will be ignored, only provided for KNode
      // compatibility.
      void setQuoteColor( const QColor &normalColor,
                          const QColor &quoteDepth1,
                          const QColor &quoteDepth2,
                          const QColor &quoteDepth3,
                          const QColor &misspelledColor );

      /**
       * Turns spellcheck highlighting on or off.
       *
       * @param on if true, spelling mistakes will be highlighted
       */
      void toggleSpellHighlighting( bool on );

      // Reimplemented to highlight quote blocks.
      virtual void highlightBlock ( const QString & text );

    protected:

      // Reimplemented, the base version sets the text color to black, which
      // is not what we want. We do nothing, the format is already reset by
      // Qt.
      virtual void unsetMisspelled( int start,  int count );

      // Reimplemented to set the color of the misspelled word to a color
      // defined by setQuoteColor().
      virtual void setMisspelled( int start, int count );

    private:
      class KEmailQuotingHighlighterPrivate;
      KEmailQuotingHighlighterPrivate *const d;

    };
}

#endif // LIBKDEPIM_KEMAILQUOTINGHIGHTER_H
