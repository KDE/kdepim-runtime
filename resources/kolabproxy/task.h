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

#ifndef KOLAB_TASK_H
#define KOLAB_TASK_H

#include <incidence.h>

#include <kcalcore/todo.h>
#include <kcalcore/incidence.h>

class QDomElement;

namespace KCal {
  class ResourceKolab;
}

namespace Kolab {

/**
 * This class represents a task, and knows how to load/save it
 * from/to XML, and from/to a KCalCore::Todo.
 * The instances of this class are temporary, only used to convert
 * one to the other.
 */
class Task : public Incidence {
public:
  /// Use this to parse an xml string to a task entry
  /// The caller is responsible for deleting the returned task
  static KCalCore::Todo::Ptr xmlToTask( const QString& xml, const QString& tz/*, KCalCore::ResourceKolab *res = 0,
                                const QString& subResource = QString(), quint32 sernum = 0 */);

  /// Use this to get an xml string describing this task entry
  static QString taskToXML( const KCalCore::Todo::Ptr &, const QString& tz );

  explicit Task( /*KCalCore::ResourceKolab *res, const QString& subResource, quint32 sernum,*/
    const QString& tz, const KCalCore::Todo::Ptr &todo = KCalCore::Todo::Ptr() );
  virtual ~Task();

  virtual QString type() const { return "Task"; }

  void saveTo( const KCalCore::Todo::Ptr &todo );

  virtual void setPriority( int priority );
  virtual int priority() const;

  virtual void setPercentCompleted( int percent );
  virtual int percentCompleted() const;

  virtual void setStatus( KCalCore::Incidence::Status status );
  virtual KCalCore::Incidence::Status status() const;

  virtual void setParent( const QString& parentUid );
  virtual QString parent() const;

  virtual void setHasStartDate( bool );
  virtual bool hasStartDate() const;

  virtual void setDueDate( const KDateTime& date );
  virtual void setDueDate( const QString &date );
  virtual void setDueDate( const QDate &date );
  virtual KDateTime dueDate() const;
  virtual bool hasDueDate() const;

  virtual void setCompletedDate( const KDateTime& date );
  virtual KDateTime completedDate() const;
  virtual bool hasCompletedDate() const;

  // Load the attributes of this class
  virtual bool loadAttribute( QDomElement& );

  // Save the attributes of this class
  virtual bool saveAttributes( QDomElement& ) const;

  // Load this task by reading the XML file
  virtual bool loadXML( const QDomDocument& xml );

  // Serialize this task to an XML string
  virtual QString saveXML() const;

protected:
  // Read all known fields from this ical todo
  void setFields( const KCalCore::Todo::Ptr & );

  // This sets the priority of this task by looking at mKolabPriorityFromDom and
  // mKCalPriorityFromDom.
  void decideAndSetPriority();

  // This is the KCal priority, not the Kolab priority.
  // See kcalPriorityToKolab() and kolabPrioritytoKCal().
  int mPriority;

  // Those priority values are the raw values read by loadAttribute().
  // They will be converted later in decideAndSetPriority().
  int mKolabPriorityFromDom;
  int mKCalPriorityFromDom;

  int mPercentCompleted;
  KCalCore::Incidence::Status mStatus;
  QString mParent;

  bool mHasStartDate;

  bool mHasDueDate;
  KDateTime mDueDate;

  bool mHasCompletedDate;
  KDateTime mCompletedDate;
};

}

#endif // KOLAB_TASK_H
