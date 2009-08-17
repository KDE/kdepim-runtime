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

#include "jobtrackermodel.h"
#include "jobtracker.h"

#include <KGlobal>
#include <KLocale>
#include <QtCore/QStringList>
#include <QtCore/QModelIndex>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QFont>
#include <QPair>

#include <cassert>

class JobTrackerModel::Private
{
public:
  Private( const char *name, JobTrackerModel* _q )
  :tracker( name), q(_q)
  {

  }

  int rowForParentId( int parentid )
  {
      const int grandparentid = tracker.parentId( parentid );
      int row = -1;
      if ( grandparentid == -1 )
      {
        row = tracker.sessions().indexOf( tracker.sessionForId( parentid ) );
      }
      else
      {
        // offset of the parent in the list of children of the grandparent
        row = tracker.jobs( grandparentid ).indexOf( tracker.info( parentid ) );
      }
      return row;
  }

  JobTracker tracker;
private:
  JobTrackerModel* const q;
};

JobTrackerModel::JobTrackerModel( const char *name, QObject* parent )
:QAbstractItemModel( parent ), d( new Private( name, this ) )
{
  connect( &d->tracker, SIGNAL( reset() ),
           this, SIGNAL( modelReset() ) );
  connect( &d->tracker, SIGNAL( added( QList< QPair< int, int> > ) ),
           this, SLOT( jobsAdded( QList< QPair< int, int> > ) ) );
    connect( &d->tracker, SIGNAL( updated( QList< QPair< int, int> > ) ),
           this, SLOT( jobsUpdated( QList< QPair< int, int> > ) ) );
}

JobTrackerModel::~JobTrackerModel()
{
  delete d;
}

QModelIndex JobTrackerModel::index(int row, int column, const QModelIndex & parent) const
{
  if ( !parent.isValid() ) // session, at top level
  {
    if ( row < 0 || row >= d->tracker.sessions().size() )
      return QModelIndex();
    return createIndex( row, column, d->tracker.idForSession( d->tracker.sessions().at(row) ) );
  }
  // non-toplevel job
  const QList<JobInfo> jobs = d->tracker.jobs( parent.internalId() );
  if ( row >= jobs.size() ) return QModelIndex();
  return createIndex( row, column, d->tracker.idForJob( jobs.at( row ).id ) );
}

QModelIndex JobTrackerModel::parent(const QModelIndex & idx) const
{
  if ( !idx.isValid() ) return QModelIndex();

  const int parentid = d->tracker.parentId( idx.internalId() );
  if ( parentid == -1 ) return QModelIndex(); // top level session

  const int row = d->rowForParentId( parentid );
  return createIndex( row, 0, parentid );
}

int JobTrackerModel::rowCount(const QModelIndex & parent) const
{
  if ( !parent.isValid() )
  {
    return d->tracker.sessions().size();
  }
  else
  {
    return d->tracker.jobs( parent.internalId() ).size();
  }
}

int JobTrackerModel::columnCount(const QModelIndex & parent) const
{
  Q_UNUSED( parent );
  return 4;
}

QVariant JobTrackerModel::data(const QModelIndex & idx, int role) const
{
  // top level items are sessions
  if ( !idx.parent().isValid() )
  {
    if ( role == Qt::DisplayRole )
    {
      if( idx.column() == 0 )
      {
        assert(d->tracker.sessions().size() > idx.row());
        return d->tracker.sessions().at(idx.row());
      }
    }
  }
  else // not top level, so a job or subjob
  {
    const int id = idx.internalId();
    const JobInfo info = d->tracker.info( id );
    if ( role == Qt::DisplayRole )
    {
      if ( idx.column() == 0 )
        return info.id;
      if ( idx.column() == 1 )
        return KGlobal::locale()->formatTime( info.timestamp.time(), true )
          + QString::fromLatin1( ".%1" ).arg( info.timestamp.time().msec(), 3, 10, QLatin1Char('0') );
      if ( idx.column() == 2 )
        return info.type;
      if ( idx.column() == 3 )
        return info.stateAsString();
    }
    else if ( role == Qt::ForegroundRole ) {
      if ( info.state == JobInfo::Failed )
        return Qt::red;
    }
    else if ( role == Qt::FontRole ) {
      if ( info.state == JobInfo::Running ) {
        QFont f;
        f.setBold( true );
        return f;
      }
    }
    else if ( role == Qt::ToolTipRole ) {
      if ( info.state == JobInfo::Failed )
        return info.error;
    }
  }
  return QVariant();
}

QVariant JobTrackerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if ( role == Qt::DisplayRole )
  {
    if ( orientation == Qt::Horizontal )
    {
      if ( section == 0 )
        return QLatin1String("Job ID");
      if ( section == 1 )
        return QLatin1String("Timestamp");
      if ( section == 2 )
        return QLatin1String("Job Type");
      if ( section == 3 )
        return QLatin1String("State");

    }
  }
  return QVariant();
}

void JobTrackerModel::resetTracker()
{
  d->tracker.triggerReset();
}


bool JobTrackerModel::isEnabled() const
{
  return d->tracker.isEnabled();
}

void JobTrackerModel::setEnabled( bool on )
{
  d->tracker.setEnabled( on );
}

void JobTrackerModel::jobsAdded( const QList< QPair< int, int > > & jobs )
{
  // TODO group them by parent? It's likely that multiple jobs for the same
  // parent will come in in the same batch, isn't it?
#define PAIR QPair<int, int> // the parser in foreach barfs otherwise
  Q_FOREACH( const PAIR& job, jobs ) {
    const int pos = job.first;
    const int parentId = job.second;
    QModelIndex parentIdx;
    if ( parentId != -1 )
      parentIdx = createIndex( d->rowForParentId( parentId), 0, parentId );
    beginInsertRows( parentIdx, pos, pos );
    endInsertRows();
  }
#undef PAIR
}

void JobTrackerModel::jobsUpdated( const QList< QPair< int, int > > & jobs )
{
  // TODO group them by parent? It's likely that multiple jobs for the same
  // parent will come in in the same batch, isn't it?
#define PAIR QPair<int, int> // the parser in foreach barfs otherwise
  Q_FOREACH( const PAIR& job, jobs ) {
    const int pos = job.first;
    const int parentId = job.second;
    QModelIndex parentIdx;
    if ( parentId != -1 )
      parentIdx = createIndex( d->rowForParentId( parentId), 0, parentId );
    dataChanged( index( pos, 0, parentIdx ), index( pos, 3, parentIdx )  );
  }
#undef PAIR
}

#include "jobtrackermodel.moc"
