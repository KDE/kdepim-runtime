/**
 * kemailquotinghighter.cpp
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

#include "kemailquotinghighter.h"

#include <KColorScheme>
#include <KConfig>
#include <KDebug>
#include <KLocale>
#include <sonnet/speller.h>

#include <QTextEdit>
#include <QTimer>
#include <QColor>
#include <QHash>
#include <QTextCursor>

namespace KPIM {
class KEMailQuotingHighlighter::KEmailQuotingHighlighterPrivate
{
public:
    QColor col1, col2, col3, misspelledColor;
    bool spellCheckingEnabled;
};

KEMailQuotingHighlighter::KEMailQuotingHighlighter( QTextEdit *textEdit,
                                                    const QColor &normalColor,
                                                    const QColor &quoteDepth1,
                                                    const QColor &quoteDepth2,
                                                    const QColor &quoteDepth3,
                                                    const QColor &misspelledColor )
    : Highlighter( textEdit ),
      d( new KEmailQuotingHighlighterPrivate() )
{
    Q_UNUSED( normalColor );
    // Don't automatically disable the spell checker, for example because there
    // are too many misspelled words. That would also disable quote highlighting.
    setAutomatic( false );

    setActive( true );
    d->col1 = quoteDepth1;
    d->col2 = quoteDepth2;
    d->col3 = quoteDepth3;
    d->misspelledColor = misspelledColor;
    d->spellCheckingEnabled = false;
}

KEMailQuotingHighlighter::~KEMailQuotingHighlighter()
{
    delete d;
}

void KEMailQuotingHighlighter::setQuoteColor( const QColor &normalColor,
                                              const QColor &quoteDepth1,
                                              const QColor &quoteDepth2,
                                              const QColor &quoteDepth3,
                                              const QColor &misspelledColor )
{
    Q_UNUSED( normalColor );
    d->col1 = quoteDepth1;
    d->col2 = quoteDepth2;
    d->col3 = quoteDepth3;
    d->misspelledColor = misspelledColor;
}

void KEMailQuotingHighlighter::toggleSpellHighlighting( bool on )
{
    if ( on != d->spellCheckingEnabled ) {
        d->spellCheckingEnabled = on;
        rehighlight();
    }
}

void KEMailQuotingHighlighter::highlightBlock( const QString & text )
{
    QString simplified = text;
    simplified = simplified.replace( QRegExp( "\\s" ), QString() )
                           .replace( '|', QLatin1String(">") );
    while ( simplified.startsWith( QLatin1String(">>>>") ) )
        simplified = simplified.mid( 3 );
    if ( simplified.startsWith( QLatin1String(">>>") ) ||
         simplified.startsWith( QLatin1String("> > >") ) )
        setFormat( 0, text.length(), d->col1 );
    else if ( simplified.startsWith( QLatin1String(">>") ) ||
              simplified.startsWith( QLatin1String("> >") ) )
        setFormat( 0, text.length(), d->col2 );
    else if ( simplified.startsWith( QLatin1String(">") ) )
        setFormat( 0, text.length(), d->col3 );
    else
    {
        if ( d->spellCheckingEnabled )
            Highlighter::highlightBlock( text );
    }
    setCurrentBlockState( 0 );
}

void KEMailQuotingHighlighter::unsetMisspelled( int start,  int count )
{
    Q_UNUSED( start )
    Q_UNUSED( count )
}

void KEMailQuotingHighlighter::setMisspelled( int start, int count )
{
    setFormat( start, count, d->misspelledColor );
}

}
