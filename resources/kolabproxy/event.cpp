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

#include "event.h"

#include <kcal/event.h>
#include <kdebug.h>

using namespace Kolab;


KCal::Event* Event::xmlToEvent( const QString& xml, const QString& tz /*, KCal::ResourceKolab* res,
                                const QString& subResource, quint32 sernum */)
{
  Event event( /*res, subResource, sernum, */tz );
  event.load( xml );
  KCal::Event* kcalEvent = new KCal::Event();
  event.saveTo( kcalEvent );
  return kcalEvent;
}

QString Event::eventToXML( KCal::Event* kcalEvent, const QString& tz  )
{
  Event event( /*0, QString(), 0, */tz, kcalEvent );
  return event.saveXML();
}

Event::Event( /*KCal::ResourceKolab *res,const QString &subResource, quint32 sernum,*/
              const QString& tz, KCal::Event* event )
  : Incidence( /*res, subResource, sernum, */tz, event ),
  mShowTimeAs( KCal::Event::Opaque ), mHasEndDate( false )
{
  if ( event )
    setFields( event );
}

Event::~Event()
{
}

void Event::setTransparency( KCal::Event::Transparency transparency )
{
  mShowTimeAs = transparency;
}

KCal::Event::Transparency Event::transparency() const
{
  return mShowTimeAs;
}

void Event::setEndDate( const KDateTime& date )
{
  mEndDate = date;
  mHasEndDate = true;
  if ( mFloatingStatus == AllDay )
    kDebug() <<"ERROR: Time on end date but no time on the event";
  mFloatingStatus = HasTime;
}

void Event::setEndDate( const QDate& date )
{
  mEndDate = KDateTime( date );
  mHasEndDate = true;
  if ( mFloatingStatus == HasTime )
    kDebug() <<"ERROR: No time on end date but time on the event";
  mFloatingStatus = AllDay;
}

void Event::setEndDate( const QString& endDate )
{
  if ( endDate.length() > 10 )
    // This is a date + time
    setEndDate( stringToDateTime( endDate ) );
  else
    // This is only a date
    setEndDate( stringToDate( endDate ) );
}

KDateTime Event::endDate() const
{
  return mEndDate;
}

bool Event::loadAttribute( QDomElement& element )
{
  // This method doesn't handle the color-label tag yet
  QString tagName = element.tagName();

  if ( tagName == "show-time-as" ) {
    // TODO: Support tentative and outofoffice
    if ( element.text() == "free" )
      setTransparency( KCal::Event::Transparent );
    else
      setTransparency( KCal::Event::Opaque );
  } else if ( tagName == "end-date" )
    setEndDate( element.text() );
  else
    return Incidence::loadAttribute( element );

  // We handled this
  return true;
}

bool Event::saveAttributes( QDomElement& element ) const
{
  // Save the base class elements
  Incidence::saveAttributes( element );

  // TODO: Support tentative and outofoffice
  if ( transparency() == KCal::Event::Transparent )
    writeString( element, "show-time-as", "free" );
  else
    writeString( element, "show-time-as", "busy" );
  if ( mHasEndDate ) {
    if ( mFloatingStatus == HasTime )
      writeString( element, "end-date", dateTimeToString( endDate() ) );
    else
      writeString( element, "end-date", dateToString( endDate().date() ) );
  }

  return true;
}


bool Event::loadXML( const QDomDocument& document )
{
  QDomElement top = document.documentElement();

  if ( top.tagName() != "event" ) {
    qWarning( "XML error: Top tag was %s instead of the expected event",
              top.tagName().toAscii().data() );
    return false;
  }

  for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      loadAttribute( e );
    } else
      kDebug() <<"Node is not a comment or an element???";
  }

//   loadAttachments();
  return true;
}

QString Event::saveXML() const
{
  QDomDocument document = domTree();
  QDomElement element = document.createElement( "event" );
  element.setAttribute( "version", "1.0" );
  saveAttributes( element );
  document.appendChild( element );
  return document.toString();
}

void Event::setFields( const KCal::Event* event )
{
  Incidence::setFields( event );

  if ( event->hasEndDate() ) {
    if ( event->allDay() ) {
      // This is an all-day event. Don't timezone move this one
      mFloatingStatus = AllDay;
      setEndDate( event->dtEnd().date() );
    } else {
      mFloatingStatus = HasTime;
      setEndDate( localToUTC( event->dtEnd() ) );
    }
  } else
    mHasEndDate = false;
  setTransparency( event->transparency() );
}

void Event::saveTo( KCal::Event* event )
{
  Incidence::saveTo( event );

  event->setHasEndDate( mHasEndDate );
  if ( mHasEndDate ) {
    if ( mFloatingStatus == AllDay )
      // This is an all-day event. Don't timezone move this one
      event->setDtEnd( endDate() );
    else
      event->setDtEnd( utcToLocal( endDate() ) );
  }
  event->setTransparency( transparency() );
}
