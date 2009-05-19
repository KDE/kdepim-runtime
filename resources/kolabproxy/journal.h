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

#ifndef KOLAB_JOURNAL_H
#define KOLAB_JOURNAL_H

#include <kolabbase.h>

class QDomElement;

namespace KCal {
  class Journal;
}

namespace Kolab {

/**
 * This class represents a journal entry, and knows how to load/save it
 * from/to XML, and from/to a KCal::Journal.
 * The instances of this class are temporary, only used to convert
 * one to the other.
 */
class Journal : public KolabBase {
public:
  /// Use this to parse an xml string to a journal entry
  /// The caller is responsible for deleting the returned journal
  static KCal::Journal* xmlToJournal( const QString& xml, const QString& tz );

  /// Use this to get an xml string describing this journal entry
  static QString journalToXML( KCal::Journal*, const QString& tz );

  explicit Journal( const QString& tz, KCal::Journal* journal = 0 );
  virtual ~Journal();

  virtual QString type() const { return "Journal"; }

  void saveTo( KCal::Journal* journal );

  virtual void setSummary( const QString& summary );
  virtual QString summary() const;

  virtual void setStartDate( const KDateTime& startDate );
  virtual KDateTime startDate() const;

  virtual void setEndDate( const KDateTime& endDate );
  virtual KDateTime endDate() const;

  // Load the attributes of this class
  virtual bool loadAttribute( QDomElement& );

  // Save the attributes of this class
  virtual bool saveAttributes( QDomElement& ) const;

  // Load this journal by reading the XML file
  virtual bool loadXML( const QDomDocument& xml );

  // Serialize this journal to an XML string
  virtual QString saveXML() const;

protected:
  // Read all known fields from this ical journal
  void setFields( const KCal::Journal* );

  QString productID() const;

  QString mSummary;
  KDateTime mStartDate;
  KDateTime mEndDate;
};

}

#endif // KOLAB_JOURNAL_H
