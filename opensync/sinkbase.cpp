/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "sinkbase.h"
#include <KDebug>

#define WRAP(X) \
  osync_trace( TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx); \
  SinkBase *sink = reinterpret_cast<SinkBase*>( \
    osync_objtype_sink_get_userdata( osync_plugin_info_get_sink( info ) ) ); \
  sink->setContext( ctx ); \
  sink->setPluginInfo( info ); \
  sink->X; \
  osync_trace( TRACE_EXIT, "%s", __PRETTY_FUNCTION__ );

extern "C"
{

static void connect_wrapper( void *userdata, OSyncPluginInfo *info, OSyncContext *ctx )
{
  WRAP( connect() )
}

static void disconnect_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
  WRAP( disconnect() )
}

static void get_changes_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
  WRAP( getChanges() )
}

static void commit_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg)
{
kDebug() << "commitwrap";
  WRAP( commit( chg ) )
}

static void sync_done_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
  WRAP( syncDone() )
}

} // extern C


SinkBase::SinkBase( int features ) :
    mContext( 0 ),
    mSink( 0 ),
    mPluginInfo( 0 )
{
  mWrapedFunctions.connect = ( features & Connect ) ? connect_wrapper : 0;
  mWrapedFunctions.disconnect = ( features & Disconnect ) ? disconnect_wrapper : 0;
  mWrapedFunctions.get_changes = ( features & GetChanges ) ? get_changes_wrapper : 0;
  mWrapedFunctions.commit = ( features & Commit ) ? commit_wrapper : 0;
  mWrapedFunctions.write = ( features & Write ) ? 0 : 0;
  mWrapedFunctions.committed_all = ( features & CommittedAll ) ? 0 : 0;
  mWrapedFunctions.read = ( features & Read ) ? 0 : 0;
  mWrapedFunctions.batch_commit = ( features & BatchCommit ) ? 0 : 0;
  mWrapedFunctions.sync_done = ( features & SyncDone ) ? sync_done_wrapper : 0;
}

SinkBase::~SinkBase()
{
  if( mSink )
    osync_objtype_sink_unref( mSink );
}

void SinkBase::connect()
{
  Q_ASSERT( false );
}

void SinkBase::disconnect()
{
  Q_ASSERT( false );
}

void SinkBase::getChanges()
{
  Q_ASSERT( false );
}

void SinkBase::commit(OSyncChange * chg)
{
  Q_UNUSED( chg );
  Q_ASSERT( false );
}

void SinkBase::syncDone()
{
  Q_ASSERT( false );
}

void SinkBase::success() const
{
kDebug();
  Q_ASSERT( mContext );
  osync_context_report_success( mContext );
  mContext = 0;
}

void SinkBase::error(OSyncErrorType type, const QString &msg) const
{
kDebug();
  Q_ASSERT( mContext );
  osync_context_report_error( mContext, type, msg.toUtf8() );
  mContext = 0;
}

void SinkBase::warning(OSyncError * error) const
{
kDebug();
  Q_ASSERT( mContext );
  osync_context_report_osyncwarning( mContext, error );
  osync_error_unref( &error );
}

void SinkBase::wrapSink(OSyncObjTypeSink * sink)
{
kDebug();
  Q_ASSERT( sink );
  Q_ASSERT( mSink == 0 );
  mSink = sink;

  osync_objtype_sink_set_functions( sink, mWrapedFunctions, this );
}

void SinkBase::setPluginInfo(OSyncPluginInfo * info)
{
  Q_ASSERT( mPluginInfo == 0 || mPluginInfo == info );
  mPluginInfo = info;
}

void SinkBase::setContext(OSyncContext * context)
{
  Q_ASSERT( mContext == 0 || context == mContext );
  // ### do I need to ref() that here? and then probably deref() the old one somewhere above?
  mContext = context;
}

#include "sinkbase.moc"
