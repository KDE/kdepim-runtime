/*
 This file is part of Akonadi.

 Copyright (c) 2009 Till Adam <adam@kde.org>

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
  JobTracker( const char *name, QObject* parent = 0 );
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
  void updated();

public Q_SLOTS:
  Q_SCRIPTABLE void jobCreated( const QString & session, const QString & job, const QString& parentJob, const QString & jobType );
  Q_SCRIPTABLE void jobStarted( const QString & job );
  Q_SCRIPTABLE void jobEnded( const QString & job, const QString &error );
  Q_SCRIPTABLE void reset();
  Q_SCRIPTABLE void setEnabled( bool on );

private:
  class Private;
  Private * const d;
};

#endif /* JOBTRACKER_H_ */
