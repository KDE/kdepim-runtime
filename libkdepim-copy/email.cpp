/*  -*- mode: C++; c-file-style: "gnu" -*-

    This file is part of kdepim.
    Copyright (c) 2004 KDEPIM developers

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "email.h"
#include <kdebug.h>

//-----------------------------------------------------------------------------
QStringList KPIM::splitEmailAddrList(const QString& aStr)
{
  // Features:
  // - always ignores quoted characters
  // - ignores everything (including parentheses and commas)
  //   inside quoted strings
  // - supports nested comments
  // - ignores everything (including double quotes and commas)
  //   inside comments

  QStringList list;

  if (aStr.isEmpty())
    return list;

  QString addr;
  uint addrstart = 0;
  int commentlevel = 0;
  bool insidequote = false;

  for (uint index=0; index<aStr.length(); index++) {
    // the following conversion to latin1 is o.k. because
    // we can safely ignore all non-latin1 characters
    switch (aStr[index].latin1()) {
    case '"' : // start or end of quoted string
      if (commentlevel == 0)
        insidequote = !insidequote;
      break;
    case '(' : // start of comment
      if (!insidequote)
        commentlevel++;
      break;
    case ')' : // end of comment
      if (!insidequote) {
        if (commentlevel > 0)
          commentlevel--;
        else {
          kdDebug(5300) << "Error in address splitting: Unmatched ')'"
                        << endl;
          return list;
        }
      }
      break;
    case '\\' : // quoted character
      index++; // ignore the quoted character
      break;
    case ',' :
      if (!insidequote && (commentlevel == 0)) {
        addr = aStr.mid(addrstart, index-addrstart);
        if (!addr.isEmpty())
          list += addr.simplifyWhiteSpace();
        addrstart = index+1;
      }
      break;
    }
  }
  // append the last address to the list
  if (!insidequote && (commentlevel == 0)) {
    addr = aStr.mid(addrstart, aStr.length()-addrstart);
    if (!addr.isEmpty())
      list += addr.simplifyWhiteSpace();
  }
  else
    kdDebug(5300) << "Error in address splitting: "
                  << "Unexpected end of address list"
                  << endl;

  return list;
}

//-----------------------------------------------------------------------------
QCString KPIM::getEmailAddr(const QString& aStr)
{
  int a, i, j, len, found = 0;
  QChar c;
  // Find the '@' in the email address:
  a = aStr.find('@');
  if (a<0) return aStr.latin1();
  // Loop backwards until we find '<', '(', ' ', or beginning of string.
  for (i = a - 1; i >= 0; i--) {
    c = aStr[i];
    if (c == '<' || c == '(' || c == ' ') found = 1;
    if (found) break;
  }
  // Reset found for next loop.
  found = 0;
  // Loop forwards until we find '>', ')', ' ', or end of string.
  for (j = a + 1; j < (int)aStr.length(); j++) {
    c = aStr[j];
    if (c == '>' || c == ')' || c == ' ') found = 1;
    if (found) break;
  }
  // Calculate the length and return the result.
  len = j - (i + 1);
  return aStr.mid(i+1,len).latin1();
}

bool KPIM::getNameAndMail(const QString& aStr, QString& name, QString& mail)
{
  name = QString::null;
  mail = QString::null;

  const int len=aStr.length();
  const char cQuotes = '"';
  
  bool bInComment, bInQuotesOutsideOfEmail;
  int i=0, iAd=0, iMailStart=0, iMailEnd=0;
  QChar c;

  // Find the '@' of the email address
  // skipping all '@' inside "(...)" comments:
  bInComment = false;
  while( i < len ){
    c = aStr[i];
    if( !bInComment ){
      if( '(' == c ){
        bInComment = true;
      }else{
        if( '@' == c ){
          iAd = i;
          break; // found it
        }
      }
    }else{
      if( ')' == c ){
        bInComment = false;
      }
    }
    ++i;
  }

  if( !iAd ){
    // We suppose the user is typing the string manually and just
    // has not finished typing the mail address part.
    // So we take everything that's left of the '<' as name and the rest as mail
    for( i = 0; len > i; ++i ) {
      c = aStr[i];
      if( '<' != c )
        name.append( c );
      else
        break;
    }
    mail = aStr.mid( i+1 );

  }else{

    // Loop backwards until we find the start of the string
    // or a ',' that is outside of a comment
    //          and outside of quoted text before the leading '<'.
    bInComment = false;
    bInQuotesOutsideOfEmail = false;
    for( i = iAd-1; 0 <= i; --i ) {
      c = aStr[i];
      if( bInComment ){
        if( '(' == c ){
          if( !name.isEmpty() )
            name.prepend( ' ' );
          bInComment = false;
        }else{
          name.prepend( c ); // all comment stuff is part of the name
        }
      }else if( bInQuotesOutsideOfEmail ){
        if( cQuotes == c )
          bInQuotesOutsideOfEmail = false;
        name.prepend( c );
      }else{
        // found the start of this addressee ?
        if( ',' == c )
          break;
        // stuff is before the leading '<' ?
        if( iMailStart ){
          if( cQuotes == c )
            bInQuotesOutsideOfEmail = true; // end of quoted text found
          name.prepend( c );
        }else{
          switch( c ){
            case '<':
              iMailStart = i;
              break;
            case ')':
              if( !name.isEmpty() )
                name.prepend( ' ' );
              bInComment = true;
              break;
            default:
              if( ' ' != c )
                mail.prepend( c );
          }
        }
      }
    }

    name = name.simplifyWhiteSpace();
    mail = mail.simplifyWhiteSpace();

    if( mail.isEmpty() )
      return false;

    mail.append('@');

    // Loop forward until we find the end of the string
    // or a ',' that is outside of a comment 
    //          and outside of quoted text behind the trailing '>'.
    bInComment = false;
    bInQuotesOutsideOfEmail = false;
    for( i = iAd+1; len > i; ++i ) {
      c = aStr[i];
      if( bInComment ){
        if( ')' == c ){
          if( !name.isEmpty() )
            name.append( ' ' );
          bInComment = false;
        }else{
          name.append( c ); // all comment stuff is part of the name
        }
      }else if( bInQuotesOutsideOfEmail ){
        if( cQuotes == c )
          bInQuotesOutsideOfEmail = false;
        name.append( c );
      }else{
        // found the end of this addressee ?
        if( ',' == c )
          break;
        // stuff is behind the trailing '>' ?
        if( iMailEnd ){
          if( cQuotes == c )
            bInQuotesOutsideOfEmail = true; // start of quoted text found
          name.append( c );
        }else{
          switch( c ){
            case '>':
              iMailEnd = i;
              break;
            case '(':
              if( !name.isEmpty() )
                name.append( ' ' );
              bInComment = true;
              break;
            default:
              if( ' ' != c )
                mail.append( c );
          }
        }
      }
    }
  }

  name = name.simplifyWhiteSpace();
  mail = mail.simplifyWhiteSpace();
  
  return ! (name.isEmpty() || mail.isEmpty());
}

bool KPIM::compareEmail( const QString& email1, const QString& email2,
                         bool matchName )
{
  QString e1Name, e1Email, e2Name, e2Email;

  getNameAndMail( email1, e1Name, e1Email );
  getNameAndMail( email2, e2Name, e2Email );

  return e1Email == e2Email &&
    ( !matchName || ( e1Name == e2Name ) );
}
