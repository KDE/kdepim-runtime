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

#ifndef KOLAB_EVENT_H
#define KOLAB_EVENT_H

#include "incidence.h"

#include <kcal/event.h>

class QDomElement;


namespace Kolab {

/**
 * This class represents an event, and knows how to load/save it
 * from/to XML, and from/to a KCal::Event.
 * The instances of this class are temporary, only used to convert
 * one to the other.
 */
class Event : public Incidence {
public:
  /// Use this to parse an xml string to a event entry
  /// The caller is responsible for deleting the returned event
  static KCal::Event* xmlToEvent( const QString& xml, const QString& tz);

  /// Use this to get an xml string describing this event entry
  static QString eventToXML( KCal::Event*, const QString& tz );

  /// Create a event object and
  explicit Event( const QString& tz, KCal::Event* event = 0 );
  virtual ~Event();

  void saveTo( KCal::Event* event );

  virtual QString type() const { return "Event"; }

  virtual void setTransparency( KCal::Event::Transparency transparency );
  virtual KCal::Event::Transparency transparency() const;

  virtual void setEndDate( const KDateTime& date );
  virtual void setEndDate( const QDate& date );
  virtual void setEndDate( const QString& date );
  virtual KDateTime endDate() const;

  // Load the attributes of this class
  virtual bool loadAttribute( QDomElement& );

  // Save the attributes of this class
  virtual bool saveAttributes( QDomElement& ) const;

  // Load this event by reading the XML file
  virtual bool loadXML( const QDomDocument& xml );

  // Serialize this event to an XML string
  virtual QString saveXML() const;

protected:
  // Read all known fields from this ical incidence
  void setFields( const KCal::Event* );

  KCal::Event::Transparency mShowTimeAs;
  KDateTime mEndDate;
  bool mHasEndDate;
};

}

#endif // KOLAB_EVENT_H
