/*  -*- c++ -*-
    kregexp3.cpp

    This file is part of libkdenetwork.
    Copyright (c) 2001 Marc Mutz <mutz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2, as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this library with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "kregexp3.h"

// #define DEBUG_KREGEXP3

#ifdef DEBUG_KREGEXP3
#include <kdebug.h>
#endif

QString KRegExp3::replace( const QString & str,
			   const QString & replacementStr,
			   int start, bool global )
{
  int oldpos, pos;

  //-------- parsing the replacementStr into
  //-------- literal parts and backreferences:
  QStringList     literalStrs;
  QValueList<int> backRefs;

  // Due to LTS: The regexp in unquoted form and with spaces:
  // \\ (\d) | \$ (\d) | \$ \{ (\d+) \}
  QRegExp rx( "\\\\(\\d)|\\$(\\d)|\\$\\{(\\d+)\\}" );
  QRegExp bbrx("\\\\");
  QRegExp brx("\\");

#ifdef DEBUG_KREGEXP3
  kdDebug() << "Analyzing replacementStr: \"" + replacementStr + "\"" << endl;
#endif

  oldpos = 0;
  pos = 0;
  while ( true ) {
    pos = rx.search( replacementStr, pos );
    
#ifdef DEBUG_KREGEXP3
    kdDebug() << QString("  Found match at pos %1").arg(pos) << endl;
#endif

    if ( pos < 0 ) {
      literalStrs << replacementStr.mid( oldpos )
	.replace( bbrx, "\\" )
	.replace( brx, "" );
#ifdef DEBUG_KREGEXP3
      kdDebug() << "  No more matches. Last literal is \"" + literalStrs.last() + "\"" << endl;
#endif
      break;
    } else {
      literalStrs << replacementStr.mid( oldpos, pos-oldpos )
	.replace( bbrx, "\\" )
	.replace( brx, "" );
#ifdef DEBUG_KREGEXP3
      kdDebug() << QString("  Inserting \"") + literalStrs.last() + "\" as literal." << endl;
      kdDebug() << "    Searching for corresponding digit(s):" << endl;
#endif
      for ( int i = 1 ; i < 4 ; i++ )
	if ( !rx.cap(i).isEmpty() ) {
	  backRefs << rx.cap(i).toInt();
#ifdef DEBUG_KREGEXP3
	  kdDebug() << QString("      Found %1 at position %2 in the capturedTexts.")
            .arg(backRefs.last()).arg(i) << endl;
#endif
	  break;
	}
      pos += rx.matchedLength();
#ifdef DEBUG_KREGEXP3
      kdDebug() << QString("  Setting new pos to %1.").arg(pos) << endl;
#endif
      oldpos = pos;
    }
  }

#ifdef DEBUG_KREGEXP3
  kdDebug() << "Finished the analysis of replacementStr!" << endl;
#endif
  Q_ASSERT( literalStrs.count() == backRefs.count() + 1 );

  //-------- actual construction of the
  //-------- resulting QString
  QString result = "";
  oldpos = 0;
  pos = start;

  QStringList::Iterator sIt;
  QValueList<int>::Iterator iIt;

  if ( start < 0 )
    start += str.length();

#ifdef DEBUG_KREGEXP3
  kdDebug() << "Constructing the resultant string starts now:" << endl;
#endif
  
  while ( pos < (int)str.length() ) {
    pos = search( str, pos );

#ifdef DEBUG_KREGEXP3
    kdDebug() << QString("  Found match at pos %1").arg(pos) << endl;
#endif

    if ( pos < 0 ) {
      result += str.mid( oldpos );
#ifdef DEBUG_KREGEXP3
      kdDebug() << "   No more matches. Adding trailing part from str:" << endl;
      kdDebug() << "    result == \"" + result + "\"" << endl;
#endif
      break;
    } else {
      result += str.mid( oldpos, pos-oldpos );
#ifdef DEBUG_KREGEXP3
      kdDebug() << "   Adding unchanged part from str:" << endl;
      kdDebug() << "    result == \"" + result + "\"" << endl;
#endif
      for ( sIt = literalStrs.begin(), iIt = backRefs.begin() ;
            iIt != backRefs.end() ; ++sIt, ++iIt ) {
	result += (*sIt);
#ifdef DEBUG_KREGEXP3
	kdDebug() << "   Adding literal replacement part:" << endl;
	kdDebug() << "    result == \"" + result + "\"" << endl;
#endif
	result += cap( (*iIt) );
#ifdef DEBUG_KREGEXP3
	kdDebug() << "   Adding captured string:" << endl;
	kdDebug() << "    result == \"" + result + "\"" << endl;
#endif
      }
      result += (*sIt);
#ifdef DEBUG_KREGEXP3
      kdDebug() << "   Adding literal replacement part:" << endl;
      kdDebug() << "    result == \"" + result + "\"" << endl;
#endif
    }
	if (matchedLength() == 0 && pos == 0) {
	  // if we matched the begin of the string, then better avoid endless
	  // recursion
	  result += str.mid( oldpos );
	  break;
	}
    pos += matchedLength();
#ifdef DEBUG_KREGEXP3
    kdDebug() << QString("  Setting new pos to %1.").arg(pos) << endl;
#endif
    oldpos = pos;

    if ( !global ) {
      // only replace the first occurrence, so stop here:
      result += str.mid( oldpos );
      break;
    }
  }

  return result;
}
