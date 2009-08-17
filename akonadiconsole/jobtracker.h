/*
 This file is part of Akonadi.

 Copyright (c) 2009 KDAB
 Author: Till Adam <adam@kde.org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 USA.
 */

#ifndef JOBTRACKER_H_
#define JOBTRACKER_H_

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QPair>

class JobInfo
{
public:
  JobInfo() :parent(-1)
  {}
  bool operator==( const JobInfo& other )
  {
      return id == other.id
          && parent == other.parent
          && type == other.type
          && timestamp == other.timestamp
          && state == other.state;
  }

  QString id;
  int parent;
  QString type;
  QDateTime timestamp;
  enum JobState
  {
      Initial = 0,
      Running,
      Ended,
      Failed
  };
  JobState state;
  QString error;
  QString stateAsString() const;
};

class JobTracker : public QObject
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.JobTracker" )

public:
  explicit JobTracker( const char *name, QObject* parent = 0 );
  ~JobTracker();
  QStringList sessions() const;

  /** Returns the list of (sub)jobs of a session or another job. */
  QList<JobInfo> jobs( const QString& parent ) const;
  QList<JobInfo> jobs( int id ) const;

  int idForJob( const QString& job ) const;
  QString jobForId( int id ) const;
  int idForSession( const QString& session ) const;
  QString sessionForId( int id ) const;
  int parentId( int id ) const;

  JobInfo info( const QString& job ) const;
  JobInfo info( int ) const;

  bool isEnabled() const;

Q_SIGNALS:
  /** Emitted when jobs (or sessiona) have been added to the tracker.
    * The format is a list of pairs consisting of the position of the
    * job or session relative to the parent and the id of that parent.
    * This makes it easy for the model to find and update the right
    * part of the model, for efficiency.
    */
  void added( const QList< QPair<int, int> >& additions );

    /** Emitted when jobs (or sessiona) have been updated in the tracker.
    * The format is a list of pairs consisting of the position of the
    * job or session relative to the parent and the id of that parent.
    * This makes it easy for the model to find and update the right
    * part of the model, for efficiency.
    */
  void updated( const QList< QPair<int, int> >& updates );

  void reset();

public Q_SLOTS:
  Q_SCRIPTABLE void jobCreated( const QString & session, const QString & job, const QString& parentJob, const QString & jobType );
  Q_SCRIPTABLE void jobStarted( const QString & job );
  Q_SCRIPTABLE void jobEnded( const QString & job, const QString &error );
  Q_SCRIPTABLE void triggerReset();
  Q_SCRIPTABLE void setEnabled( bool on );

private Q_SLOTS:
  void signalUpdates();

private:
  class Private;
  Private * const d;
};

#endif /* JOBTRACKER_H_ */
