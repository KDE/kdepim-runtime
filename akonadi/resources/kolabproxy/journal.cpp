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

#include "journal.h"
#include "kdepim-version.h"

#include <kcal/journal.h>
#include <kdebug.h>

static const char korgVersion[] = KDEPIM_VERSION;

using namespace Kolab;


KCal::Journal* Journal::xmlToJournal( const QString& xml, const QString& tz )
{
  Journal journal( tz );
  journal.load( xml );
  KCal::Journal* kcalJournal = new KCal::Journal();
  journal.saveTo( kcalJournal );
  return kcalJournal;
}

QString Journal::journalToXML( KCal::Journal* kcalJournal, const QString& tz )
{
  Journal journal( tz, kcalJournal );
  return journal.saveXML();
}

Journal::Journal( const QString& tz, KCal::Journal* journal )
  : KolabBase( tz )
{
  if ( journal )
    setFields( journal );
}

Journal::~Journal()
{
}

void Journal::setSummary( const QString& summary )
{
  mSummary = summary;
}

QString Journal::summary() const
{
  return mSummary;
}

void Journal::setStartDate( const KDateTime& startDate )
{
  mStartDate = startDate;
}

KDateTime Journal::startDate() const
{
  return mStartDate;
}

void Journal::setEndDate( const KDateTime& endDate )
{
  mEndDate = endDate;
}

KDateTime Journal::endDate() const
{
  return mEndDate;
}

bool Journal::loadAttribute( QDomElement& element )
{
  QString tagName = element.tagName();

  if ( tagName == "summary" )
    setSummary( element.text() );
  else if ( tagName == "start-date" )
    setStartDate( stringToDateTime( element.text() ) );
  else
    // Not handled here
    return KolabBase::loadAttribute( element );

  // We handled this
  return true;
}

bool Journal::saveAttributes( QDomElement& element ) const
{
  // Save the base class elements
  KolabBase::saveAttributes( element );

  writeString( element, "summary", summary() );
  writeString( element, "start-date", dateTimeToString( startDate() ) );

  return true;
}


bool Journal::loadXML( const QDomDocument& document )
{
  QDomElement top = document.documentElement();

  if ( top.tagName() != "journal" ) {
    qWarning( "XML error: Top tag was %s instead of the expected Journal",
              top.tagName().toAscii().data() );
    return false;
  }

  for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      if ( !loadAttribute( e ) ) {
        // Unhandled tag - save for later storage
        //qDebug( "Unhandled tag: %s", e.toCString().data() );
      }
    } else
      qDebug( "Node is not a comment or an element???" );
  }

  return true;
}

QString Journal::saveXML() const
{
  QDomDocument document = domTree();
  QDomElement element = document.createElement( "journal" );
  element.setAttribute( "version", "1.0" );
  saveAttributes( element );
  document.appendChild( element );
  return document.toString();
}

void Journal::saveTo( KCal::Journal* journal )
{
  KolabBase::saveTo( journal );

  journal->setSummary( summary() );
  journal->setDtStart( utcToLocal( startDate() ) );
}

void Journal::setFields( const KCal::Journal* journal )
{
  // Set baseclass fields
  KolabBase::setFields( journal );

  // Set our own fields
  setSummary( journal->summary() );
  setStartDate( localToUTC( journal->dtStart() ) );
}

QString Journal::productID() const
{
  return QString( "KOrganizer " ) + korgVersion + ", Kolab resource";
}
