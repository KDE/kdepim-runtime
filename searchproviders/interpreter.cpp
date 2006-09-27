/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "interpreter.h"

QStringListIterator Parser::balanced( QStringListIterator _it ) const
{
  QStringListIterator it( _it );

  int stack = 0;
  while ( it.hasNext() ) {
    const QString token = it.next();
    if ( token[ 0 ] == '(' )
      stack++;
    else if ( token[ 0 ] == ')' ) {
      stack--;

      if ( stack == 0 ) {
        return it;
      }
    }
  }

  return _it;
}

QStringList Parser::tokenize( const QString &query ) const
{
  QString pattern( query.trimmed() );

  QStringList tokens;
  QString token;

  bool escaped = false;
  bool inBrackets = false;
  int i = 0;
  while ( i < pattern.count() ) {
    if ( inBrackets ) {
      if ( escaped ) {
        escaped = false;
      } else {
        if ( pattern[ i ] == '\\' ) {
          escaped = true;
          ++i;
          continue;
        }

        if ( pattern[ i ] == '\'' ) {
          inBrackets = false;
          ++i;
          continue;
        }
      }
    } else {
      if ( pattern[ i ] == '\'' ) {
        inBrackets = true;
        ++i;
        continue;
      }

      if ( pattern[ i ] == '(' ) {
        if ( !token.isEmpty() )
          tokens.append( token );

        tokens.append( "(" );
        token = QString();

        ++i;
        while ( pattern[ i ].isSpace() )
          ++i;

        continue;
      }

      if ( pattern[ i ] == ')' ) {
        if ( !token.isEmpty() )
          tokens.append( token );

        tokens.append( ")" );
        token = QString();

        ++i;
        while ( pattern[ i ].isSpace() )
          ++i;

        continue;
      }

      // we found a separator
      if ( pattern[ i ].isSpace() ) {
        tokens.append( token );
        token = QString();

        while ( pattern[ i ].isSpace() )
          ++i;

        continue;
      }
    }

    token.append( pattern[ i ] );
    ++i;
  }

  if ( !token.isEmpty() )
    tokens.append( token );

  return tokens;
}
