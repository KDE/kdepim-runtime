/*
    This file is part of the kolab resource - the implementation of the
    Kolab storage format. See www.kolab.org for documentation on this.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

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

#include "note.h"
#include "libkolab-version.h"

#include <kcalcore/journal.h>
#include <QDebug>

using namespace KolabV2;


KCalCore::Journal::Ptr Note::xmlToJournal( const QString& xml )
{
  Note note;
  note.load( xml );
  KCalCore::Journal::Ptr journal( new KCalCore::Journal() );
  note.saveTo( journal );
  return journal;
}

QString Note::journalToXML( const KCalCore::Journal::Ptr &journal )
{
  Note note( journal );
  return note.saveXML();
}

Note::Note( const KCalCore::Journal::Ptr &journal ) : mRichText( false )
{
  if ( journal )
    setFields( journal );
}

Note::~Note()
{
}

void Note::setSummary( const QString& summary )
{
  mSummary = summary;
}

QString Note::summary() const
{
  return mSummary;
}

void Note::setBackgroundColor( const QColor& bgColor )
{
  mBackgroundColor = bgColor;
}

QColor Note::backgroundColor() const
{
  return mBackgroundColor;
}

void Note::setForegroundColor( const QColor& fgColor )
{
  mForegroundColor = fgColor;
}

QColor Note::foregroundColor() const
{
  return mForegroundColor;
}

void Note::setRichText( bool richText )
{
  mRichText = richText;
}

bool Note::richText() const
{
  return mRichText;
}

bool Note::loadAttribute( QDomElement& element )
{
  QString tagName = element.tagName();
  if ( tagName == QLatin1String("summary") )
    setSummary( element.text() );
  else if ( tagName == QLatin1String("foreground-color") )
    setForegroundColor( stringToColor( element.text() ) );
  else if ( tagName == QLatin1String("background-color") )
    setBackgroundColor( stringToColor( element.text() ) );
  else if ( tagName == QLatin1String("knotes-richtext") )
    mRichText = ( element.text() == QLatin1String("true") );
  else
    return KolabBase::loadAttribute( element );

  // We handled this
  return true;
}

bool Note::saveAttributes( QDomElement& element ) const
{
  // Save the base class elements
  KolabBase::saveAttributes( element );

  // Save the elements
#if 0
  QDomComment c = element.ownerDocument().createComment( "Note specific attributes" );
  element.appendChild( c );
#endif

  writeString( element, QStringLiteral("summary"), summary() );
  if ( foregroundColor().isValid() )
    writeString( element, QStringLiteral("foreground-color"), colorToString( foregroundColor() ) );
  if ( backgroundColor().isValid() )
    writeString( element, QStringLiteral("background-color"), colorToString( backgroundColor() ) );
  writeString( element, QStringLiteral("knotes-richtext"), mRichText ? QStringLiteral("true") : QStringLiteral("false") );

  return true;
}


bool Note::loadXML( const QDomDocument& document )
{
  QDomElement top = document.documentElement();

  if ( top.tagName() != QLatin1String("note") ) {
    qWarning( "XML error: Top tag was %s instead of the expected note",
              top.tagName().toAscii().data() );
    return false;
  }

  for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      if ( !loadAttribute( e ) )
        // TODO: Unhandled tag - save for later storage
        qDebug() <<"Warning: Unhandled tag" << e.tagName();
    } else
      qDebug() <<"Node is not a comment or an element???";
  }

  return true;
}

QString Note::saveXML() const
{
  QDomDocument document = domTree();
  QDomElement element = document.createElement( QStringLiteral("note") );
  element.setAttribute( QStringLiteral("version"), QStringLiteral("1.0") );
  saveAttributes( element );
  document.appendChild( element );
  return document.toString();
}

void Note::setFields( const KCalCore::Journal::Ptr &journal )
{
  KolabBase::setFields( journal );

  setSummary( journal->summary() );

  QString property = journal->customProperty( "KNotes", "BgColor" );
  if ( !property.isEmpty() ) {
    setBackgroundColor( property );
  } else {
    setBackgroundColor( QStringLiteral("yellow") );
  }

  property = journal->customProperty( "KNotes", "FgColor" );
  if ( !property.isEmpty() ) {
    setForegroundColor( property );
  } else {
    setForegroundColor( QStringLiteral("black") );
  }

  property = journal->customProperty( "KNotes", "RichText" );
  if ( !property.isEmpty() ) {
    setRichText( property == "true" ? true : false );
  } else {
    setRichText( "false" );
  }
}

void Note::saveTo( const KCalCore::Journal::Ptr &journal )
{
  KolabBase::saveTo( journal );

  // TODO: background and foreground
  journal->setSummary( summary() );
  if ( foregroundColor().isValid() )
    journal->setCustomProperty( "KNotes", "FgColor",
                                colorToString( foregroundColor() ) );
  if ( backgroundColor().isValid() )
    journal->setCustomProperty( "KNotes", "BgColor",
                                colorToString( backgroundColor() ) );
  journal->setCustomProperty( "KNotes", "RichText",
                              richText() ? "true" : "false" );
}

QString Note::productID() const
{
  return QStringLiteral( "KNotes %1, Kolab resource" ).arg( LIBKOLAB_LIB_VERSION_STRING );
}
