/**
 * linklocator.cpp
 *
 * Copyright (c) 2002 Dave Corrie <kde@davecorrie.com>
 *
 *  This file is part of KMail.
 *
 *  KMail is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "linklocator.h"
#include "pimemoticons.h"
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kstaticdeleter.h>
#include <kmdcodec.h>
#include <kdebug.h>

#include <qstylesheet.h>
#include <qfile.h>

QMap<QString, QString> *LinkLocator::s_smileyEmoticonNameMap = 0;
QMap<QString, QString> *LinkLocator::s_smileyEmoticonHTMLCache = 0;

static KStaticDeleter< QMap<QString, QString> > smileyMapDeleter;
static KStaticDeleter< QMap<QString, QString> > smileyCacheDeleter;

LinkLocator::LinkLocator(const QString& text, int pos)
  : mText(text), mPos(pos), mMaxUrlLen(4096), mMaxAddressLen(255)
{
  // If you change either of the above values for maxUrlLen or
  // maxAddressLen, then please also update the documentation for
  // setMaxUrlLen()/setMaxAddressLen() in the header file AND the
  // default values used for the maxUrlLen/maxAddressLen parameters
  // of convertToHtml().

  if ( !s_smileyEmoticonNameMap ) {
    smileyMapDeleter.setObject( s_smileyEmoticonNameMap,
                                new QMap<QString, QString>() );
    for ( int i = 0; i < EmotIcons::EnumSindex::COUNT; ++i ) {
      QString imageName( EmotIcons::EnumSindex::enumToString[i] );
      imageName.truncate( imageName.length() - 2 ); //remove the _0 bit
      s_smileyEmoticonNameMap->insert( EmotIcons::smiley(i), imageName );
    }
  }

  if ( !s_smileyEmoticonHTMLCache )
    smileyCacheDeleter.setObject( s_smileyEmoticonHTMLCache,
                                  new QMap<QString, QString>() );
}

void LinkLocator::setMaxUrlLen(int length)
{
  mMaxUrlLen = length;
}

int LinkLocator::maxUrlLen() const
{
  return mMaxUrlLen;
}

void LinkLocator::setMaxAddressLen(int length)
{
  mMaxAddressLen = length;
}

int LinkLocator::maxAddressLen() const
{
  return mMaxAddressLen;
}

QString LinkLocator::getUrl()
{
  QString url;
  if(atUrl())
  {
    // handle cases like this: <link>http://foobar.org/</link>
    int start = mPos;
    while(mPos < (int)mText.length() && mText[mPos] > ' ' && mText[mPos] != '"' &&
      QString("<>()[]").find(mText[mPos]) == -1)
    {
      ++mPos;
    }
    /* some URLs really end with:  # / &     */
    const QString allowedSpecialChars = QString("#/&");
    while(mPos > start && mText[mPos-1].isPunct() &&
		    allowedSpecialChars.find(mText[mPos-1]) == -1 )
    {
      --mPos;
    }

    url = mText.mid(start, mPos - start);
    if(isEmptyUrl(url) || mPos - start > maxUrlLen())
    {
      mPos = start;
      url = "";
    }
    else
    {
      --mPos;
    }
  }
  return url;
}

// keep this in sync with KMMainWin::slotUrlClicked()
bool LinkLocator::atUrl() const
{
  // the following characters are allowed in a dot-atom (RFC 2822):
  // a-z A-Z 0-9 . ! # $ % & ' * + - / = ? ^ _ ` { | } ~
  const QString allowedSpecialChars = QString(".!#$%&'*+-/=?^_`{|}~");

  // the character directly before the URL must not be a letter, a number or
  // any other character allowed in a dot-atom (RFC 2822).
  if( ( mPos > 0 ) && ( mText[mPos-1].isLetterOrNumber() ||
                        ( allowedSpecialChars.find( mText[mPos-1] ) != -1 ) ) )
    return false;

  QChar ch = mText[mPos];
  return (ch=='h' && ( mText.mid(mPos, 7) == "http://" ||
                       mText.mid(mPos, 8) == "https://") ) ||
         (ch=='v' && mText.mid(mPos, 6) == "vnc://") ||
         (ch=='f' && ( mText.mid(mPos, 6) == "ftp://" ||
                       mText.mid(mPos, 7) == "ftps://") ) ||
         (ch=='s' && ( mText.mid(mPos, 7) == "sftp://" ||
                       mText.mid(mPos, 6) == "smb://") ) ||
         (ch=='m' && mText.mid(mPos, 7) == "mailto:") ||
         (ch=='w' && mText.mid(mPos, 4) == "www.") ||
         (ch=='f' && mText.mid(mPos, 4) == "ftp.");
         // note: no "file:" for security reasons
}

bool LinkLocator::isEmptyUrl(const QString& url)
{
  return url.isEmpty() ||
         url == "http://" ||
         url == "https://" ||
         url == "ftp://" ||
         url == "ftps://" ||
         url == "sftp://" ||
         url == "smb://" ||
         url == "vnc://" ||
         url == "mailto" ||
         url == "www" ||
         url == "ftp";
}

QString LinkLocator::getEmailAddress()
{
  QString address;

  if(mText[mPos] == '@')
  {
    // the following characters are allowed in a dot-atom (RFC 2822):
    // a-z A-Z 0-9 . ! # $ % & ' * + - / = ? ^ _ ` { | } ~
    const QString allowedSpecialChars = QString(".!#$%&'*+-/=?^_`{|}~");

    // determine the local part of the email address
    int start = mPos - 1;
    while (start >= 0 && mText[start].unicode() < 128 &&
      (mText[start].isLetterOrNumber() ||
        mText[start] == '@' || // allow @ to find invalid email addresses
        allowedSpecialChars.find(mText[start]) != -1))
    {
      --start;
    }
    ++start;
    // we assume that an email address starts with a letter or a digit
    while (allowedSpecialChars.find(mText[start]) != -1)
      ++start;

    // determine the domain part of the email address
    int end = mPos + 1;
    while (end < (int)mText.length() &&
      (mText[end].isLetterOrNumber() ||
        mText[end] == '@' || // allow @ to find invalid email addresses
        allowedSpecialChars.find(mText[end]) != -1))
    {
      ++end;
    }
    // we assume that an email address ends with a letter or a digit
    while (allowedSpecialChars.find(mText[end - 1]) != -1)
      --end;

    address = mText.mid(start, end - start);
    if(isEmptyAddress(address) || end - start > maxAddressLen() || address.contains('@') != 1)
      address = "";

    if(!address.isEmpty())
      mPos = end - 1;
  }
  return address;
}

bool LinkLocator::isEmptyAddress(const QString& address)
{
  return address.isEmpty() ||
         address[0] == '@' ||
         address[address.length() - 1] == '@';
}

QString LinkLocator::convertToHtml(const QString& plainText, int flags,
  int maxUrlLen, int maxAddressLen)
{
  LinkLocator locator(plainText);
  locator.setMaxUrlLen(maxUrlLen);
  locator.setMaxAddressLen(maxAddressLen);

  QString str;
  QString result((QChar*)0, (int)locator.mText.length() * 2);
  QChar ch;
  int x;
  bool startOfLine = true;
  QString emoticon;

  for (locator.mPos = 0, x = 0; locator.mPos < (int)locator.mText.length(); locator.mPos++, x++)
  {
    ch = locator.mText[locator.mPos];
    if ( flags & PreserveSpaces )
    {
      if (ch==' ')
      {
        if (startOfLine) {
          result += "&nbsp;";
          locator.mPos++, x++;
          startOfLine = false;
        }
        while (locator.mText[locator.mPos] == ' ')
        {
          result += " ";
          locator.mPos++, x++;
          if (locator.mText[locator.mPos] == ' ') {
            result += "&nbsp;";
            locator.mPos++, x++;
          }
        }
        locator.mPos--, x--;
        continue;
      }
      else if (ch=='\t')
      {
        do
        {
          result += "&nbsp;";
          x++;
        }
        while((x&7) != 0);
        x--;
        startOfLine = false;
        continue;
      }
    }
    if (ch=='\n')
    {
      result += "<br />";
      startOfLine = true;
      x = -1;
      continue;
    }

    startOfLine = false;
    if (ch=='&')
      result += "&amp;";
    else if (ch=='"')
      result += "&quot;";
    else if (ch=='<')
      result += "&lt;";
    else if (ch=='>')
      result += "&gt;";
    else
    {
      const int start = locator.mPos;
      str = locator.getUrl();
      if(!str.isEmpty())
      {
        QString hyperlink;
        if(str.left(4) == "www.")
          hyperlink = "http://" + str;
        else if(str.left(4) == "ftp.")
          hyperlink = "ftp://" + str;
        else
          hyperlink = str;

	str = str.replace('&', "&amp;");
        result += "<a href=\"" + hyperlink + "\">" + str + "</a>";
        x += locator.mPos - start;
      }
      else
      {
        str = locator.getEmailAddress();
        if(!str.isEmpty())
        {
          // len is the length of the local part
          int len = str.find('@');
          QString localPart = str.left(len);

          // remove the local part from the result (as '&'s have been expanded to
          // &amp; we have to take care of the 4 additional characters per '&')
          result.truncate(result.length() - len - (localPart.contains('&')*4));
          x -= len;

          result += "<a href=\"mailto:" + str + "\">" + str + "</a>";
          x += str.length() - 1;
        }
        else {
          if ( flags & ReplaceSmileys )
            str = locator.getEmoticon();
          if ( ! str.isEmpty() ) {
            result += str;
            x += locator.mPos - start;
          }
          else {
            result += ch;
          }
        }
      }
    }
  }

  return result;
}

static QString pngToDataUrl( const QString & iconPath )
{
  if ( iconPath.isEmpty() )
    return QString::null;

  QFile pngFile( iconPath );
  if ( !pngFile.open( IO_ReadOnly | IO_Raw ) )
    return QString::null;

  QByteArray ba = pngFile.readAll();
  pngFile.close();
  return QString::fromLatin1("data:image/png;base64,%1")
         .arg( KCodecs::base64Encode( ba ) );
}


QString LinkLocator::getEmoticon()
{
  // smileys have to be prepended by whitespace
  if ( ( mPos > 0 ) && !mText[mPos-1].isSpace() )
    return QString::null;

  // since smileys start with ':', ';', '(' or '8' short circuit method
  const QChar ch = mText[mPos];
  if ( ch !=':' && ch != ';' && ch != '(' && ch != '8' )
    return QString::null;

  // find the end of the smiley (a smiley is at most 4 chars long and ends at
  // lineend or whitespace)
  const int MinSmileyLen = 2;
  const int MaxSmileyLen = 4;
  int smileyLen = 1;
  while ( ( smileyLen <= MaxSmileyLen ) &&
          ( mPos+smileyLen < (int)mText.length() ) &&
          !mText[mPos+smileyLen].isSpace() )
    smileyLen++;
  if ( smileyLen < MinSmileyLen || smileyLen > MaxSmileyLen )
    return QString::null;

  const QString smiley = mText.mid( mPos, smileyLen );
  if ( !s_smileyEmoticonNameMap->contains( smiley ) )
    return QString::null; // that's not a (known) smiley

  QString htmlRep;
  if ( s_smileyEmoticonHTMLCache->contains( smiley ) ) {
    htmlRep = (*s_smileyEmoticonHTMLCache)[smiley];
  }
  else {
    const QString imageName = (*s_smileyEmoticonNameMap)[smiley];

#if KDE_IS_VERSION( 3, 3, 91 )
    const QString iconPath = locate( "emoticons",
                                     EmotIcons::theme() +
                                     QString::fromLatin1( "/" ) +
                                     imageName + QString::fromLatin1(".png") );
#else
    const QString iconPath = locate( "data",
                                     QString::fromLatin1( "kopete/pics/emoticons/" )+
                                     EmotIcons::theme() +
                                     QString::fromLatin1( "/" ) +
                                     imageName + QString::fromLatin1(".png") );
#endif

    const QString dataUrl = pngToDataUrl( iconPath );
    if ( dataUrl.isEmpty() ) {
      htmlRep = QString::null;
    }
    else {
      // create an image tag with nested span (the text in the span is used
      // for copy & paste) representing the smiley
      htmlRep = QString("<span class=\"pimsmileytext\">%1</span>"
                        "<img class=\"pimsmileyimg\" src=\"%2\" "
                        "alt=\"%3\" title=\"%4\"/>")
                .arg( QStyleSheet::escape( smiley ), dataUrl,
                      QStyleSheet::escape( smiley ),
                      QStyleSheet::escape( smiley ) );
    }
    s_smileyEmoticonHTMLCache->insert( smiley, htmlRep );
  }

  if ( !htmlRep.isEmpty() )
    mPos += smileyLen - 1;

  return htmlRep;
}

