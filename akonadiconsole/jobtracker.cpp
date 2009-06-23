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

#include "jobtracker.h"
#include "jobtrackeradaptor.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include <cassert>

QString JobInfo::stateAsString() const
{
    switch( state )
    {
        case Initial:
            return QLatin1String("Initial");
        case Running:
            return QLatin1String("Running");
        case Ended:
            return QLatin1String("Ended");
        case Failed:
            return QString::fromLatin1( "Failed: %1" ).arg( error );
        default:
            return QLatin1String("Unknown state!");
    }
}


class JobTracker::Private
{
public:
  Private(JobTracker* _q)
  :q(_q), lastId(42), timer( _q )
  {
    timer.setSingleShot( true );
    timer.setInterval( 200 );
    connect( &timer, SIGNAL(timeout()), q, SIGNAL(updated()) );
  }

  bool isSession( int id ) const
  {
    return id < -1;
  }

  void emitUpdated()
  {
    if ( !timer.isActive() && !disabled )
      timer.start();
  }

  QStringList sessions;
  QMap<QString, int> idToSequence;
  QMap<int, QString> sequenceToId;
  QMap<QString, QStringList> jobs;
  QMap<QString, JobInfo> infos;
  int lastId;
  QTimer timer;
  bool disabled;
private:
  JobTracker* const q;
};

JobTracker::JobTracker( const char *name, QObject* parent )
:QObject( parent ), d( new Private( this ) )
{
  new JobTrackerAdaptor( this );
  QDBusConnection::sessionBus().registerService( QLatin1String("org.kde.akonadiconsole") );
  QDBusConnection::sessionBus().registerObject( '/'+QLatin1String(name),
      this, QDBusConnection::ExportAdaptors );

#if 0
    // dummy data for testing
    d->sessions << "one" << "two" << "three";
    d->jobs.insert("one", QStringList() << "eins" );
    d->jobs.insert("two", QStringList() );
    d->jobs.insert("three", QStringList() );

    // create some fake jobs
    d->jobs.insert( "eins", QStringList() << "sub-eins" << "sub-zwei" );
    d->idToSequence.insert( "eins", 0 );
    d->sequenceToId.insert( 0, "eins" );
    JobInfo info;
    info.id = "eins";
    info.parent = -2;
    d->infos.insert( "eins", info );

    d->jobs.insert( "sub-eins", QStringList() );
    d->idToSequence.insert( "sub-eins", 1 );
    d->sequenceToId.insert( 1, "sub-eins" );
    info.id = "sub-eins";
    info.parent = 0;
    d->infos.insert( "sub-eins", info );

    d->jobs.insert( "sub-zwei", QStringList() );
    d->idToSequence.insert( "sub-zwei", 2 );
    d->sequenceToId.insert( 2, "sub-zwei" );
    info.id = "sub-zwei";
    info.parent = 0;
    d->infos.insert( "sub-zwei", info );
#endif
}

JobTracker::~JobTracker()
{
  delete d;
}

void JobTracker::jobCreated( const QString & session, const QString & job, const QString & parent, const QString & jobType )
{
  if ( d->disabled || session.isEmpty() || job.isEmpty() ) return;

  if ( !parent.isEmpty() && !d->jobs.contains( parent ) )
  {
    qWarning() << "JobTracker: Job arrived before it's parent! Fix the library!";
    jobCreated( session, parent, QString(),"dummy job type" );
  }

  // check if it's a new session, if so, add it
  if (!d->sessions.contains( session ) )
  {
    d->sessions.append( session );
    d->jobs.insert( session, QStringList() );
  }

  // deal with the job
  if ( d->jobs.contains( job ) ) return; // duplicate?

  d->jobs.insert( job, QStringList() );

  JobInfo info;
  info.id = job;
  if ( parent.isEmpty() )
    info.parent = idForSession( session );
  else
    info.parent = idForJob( parent );
  info.state = JobInfo::Initial;
  info.timestamp = QDateTime::currentDateTime();
  info.type = jobType;
  d->infos.insert( job, info );
  const int id = d->lastId++;
  d->idToSequence.insert( job, id );
  d->sequenceToId.insert( id, job );

  QString daddy;
  if ( parent.isEmpty() )
    daddy = session;
  else
    daddy = parent;

  assert(!daddy.isEmpty());
  QStringList kids = d->jobs[daddy];
  kids << job;
  d->jobs[daddy] = kids;

  d->emitUpdated();
}

void JobTracker::jobEnded( const QString& job, const QString& error )
{
  // this is called from dbus, so better be defensive
  if ( d->disabled || !d->jobs.contains( job ) || !d->infos.contains( job ) ) return;

  JobInfo info = d->infos[job];
  if ( error.isEmpty() ) {
    info.state = JobInfo::Ended;
  } else {
    info.state = JobInfo::Failed;
    info.error = error;
  }
  d->infos[job] = info;

  d->emitUpdated();
}

void JobTracker::jobStarted( const QString & job )
{
  // this is called from dbus, so better be defensive
  if ( d->disabled || !d->jobs.contains( job ) || !d->infos.contains( job ) ) return;

  JobInfo info = d->infos[job];
  info.state = JobInfo::Running;
  d->infos[job] = info;

  d->emitUpdated();
}

QStringList JobTracker::sessions() const
{
  return d->sessions;
}

QList<JobInfo> JobTracker::jobs( int id ) const
{
  if ( d->isSession( id ) )
    return jobs( sessionForId( id ) );
  return jobs( jobForId( id ) );
}

QList<JobInfo> JobTracker::jobs( const QString & parent ) const
{
  assert( d->jobs.contains(parent) );
  const QStringList jobs = d->jobs[parent];
  QList<JobInfo> infos;
  Q_FOREACH( QString job, jobs )
  {
    infos << d->infos[job];
  }
  return infos;
}

// only works on jobs
int JobTracker::idForJob(const QString & job) const
{
  assert( d->idToSequence.contains(job) );
  return d->idToSequence[job];
}



QString JobTracker::jobForId(int id) const
{
  if ( d->isSession( id ) )
    return sessionForId( id );
  assert( d->sequenceToId.contains(id) );
  return d->sequenceToId[id];
}


// To find a session, we take the offset in the list of sessions
// in order of appearance, add one, and make it negative. That
// way we can discern session ids from job ids and use -1 for invalid
int JobTracker::idForSession(const QString & session) const
{
  assert( d->sessions.contains( session ) );
  return ( d->sessions.indexOf(session) + 2 ) * -1;
}

QString JobTracker::sessionForId(int _id) const
{
  const int id = (-_id)-2;
  assert( d->sessions.size() > id );
  return d->sessions.at(id);
}

int JobTracker::parentId( int id ) const
{
  if ( d->isSession( id ) )
  {
    return -1;
  }
  else
  {
    const QString job = d->sequenceToId[id];
    return d->infos[job].parent;
  }

}

JobInfo JobTracker::info( int id) const
{
  return info( jobForId( id ) );
}

JobInfo JobTracker::info(const QString & job) const
{
  assert( d->infos.contains(job) );
  return d->infos[job];
}

void JobTracker::reset()
{
  d->sessions.clear();
  d->idToSequence.clear();
  d->sequenceToId.clear();
  d->jobs.clear();
  d->infos.clear();

  emit updated();
}

void JobTracker::setEnabled( bool on )
{
  d->disabled = !on;
}

bool JobTracker::isEnabled() const
{
  return !d->disabled;
}

#include "jobtracker.moc"
