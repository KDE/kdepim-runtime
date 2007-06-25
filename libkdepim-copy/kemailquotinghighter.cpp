/**
 * kemailquotinghighter.cpp
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

#include "kemailquotinghighter.h"
#include <sonnet/loader.h>
#include <sonnet/speller.h>
#include <sonnet/settings.h>

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>

#include <QTextEdit>
#include <QTimer>
#include <QColor>
#include <QHash>
#include <QTextCursor>

namespace Sonnet {
class KEMailQuotingHighlighter::KEmailQuotingHighlighterPrivate
{
public:
    QColor col1, col2, col3, col4, col5;
    SyntaxMode mode;
    bool enabled;
};

KEMailQuotingHighlighter::KEMailQuotingHighlighter( QTextEdit *textEdit,
                                                                const QColor& depth0,
                                                                const QColor& depth1,
                                                                const QColor& depth2,
                                                                const QColor& depth3,
                                                                SyntaxMode mode )
    : Highlighter( textEdit ),d(new KEmailQuotingHighlighterPrivate())
{
    setAutomatic(false); //disable automatic de-enable highlighting
    setActive(true);
    d->col1 = depth0;
    d->col2 = depth1;
    d->col3 = depth2;
    d->col4 = depth3;
    d->col5 = depth0;

    d->mode = mode;
}

KEMailQuotingHighlighter::~KEMailQuotingHighlighter()
{
    delete d;
}

void KEMailQuotingHighlighter::setSyntaxMode( SyntaxMode mode)
{
  d->mode = mode;
}

void KEMailQuotingHighlighter::setQuoteColor( const QColor& QuoteColor0, const QColor& QuoteColor1, const QColor& QuoteColor2, const QColor& QuoteColor3)
{
    d->col1 = QuoteColor0;
    d->col2 = QuoteColor1;
    d->col3 = QuoteColor2;
    d->col4 = QuoteColor3;
    d->col5 = QuoteColor0;
}

void KEMailQuotingHighlighter::highlightBlock ( const QString & text )
{
    if ( !isActive() )
        return;	
    QString simplified = text;
    simplified = simplified.replace( QRegExp( "\\s" ), QString() ).replace( '|', QLatin1String(">") );
    while ( simplified.startsWith( QLatin1String(">>>>") ) )
	simplified = simplified.mid(3);
    if	( simplified.startsWith( QLatin1String(">>>") ) || simplified.startsWith( QString::fromLatin1("> >	>") ) )
	setFormat( 0, text.length(), d->col2 );
    else if	( simplified.startsWith( QLatin1String(">>") ) || simplified.startsWith( QString::fromLatin1("> >") ) )
	setFormat( 0, text.length(), d->col3 );
    else if	( simplified.startsWith( QLatin1String(">") ) )
	setFormat( 0, text.length(), d->col4 );
    else
    {
	setFormat( 0, text.length(), d->col5 );
        Highlighter::highlightBlock ( text );
    }
    setCurrentBlockState ( 0 );

}

}
