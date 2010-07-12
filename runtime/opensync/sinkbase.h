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

#ifndef SINKBASE_H
#define SINKBASE_H

#include <QObject>

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-error.h>
#include <opensync/opensync-helper.h>

/**
 * Base class providing wraping of OSyncObjTypeSink.
 */
class SinkBase : public QObject
{
  Q_OBJECT
  public:
    enum Feature {
      Connect = 1,
      Disconnect = 2,
      GetChanges = 4,
      Commit = 8,
      Write = 16,
      CommittedAll = 32,
      Read = 64,
      BatchCommit = 128,
      SyncDone = 256
    };

    SinkBase( int features );
    virtual ~SinkBase();

    virtual void connect();
    virtual void disconnect();
    virtual void getChanges();
    virtual void commit( OSyncChange *chg );
    virtual void syncDone();

    OSyncContext* context() const { return mContext; }
    void setContext( OSyncContext *context );

    OSyncPluginInfo *pluginInfo() const { return mPluginInfo; }
    void setPluginInfo( OSyncPluginInfo *info );

  protected:
    void success() const;
    void error( OSyncErrorType type, const QString &msg ) const;
    void warning( OSyncError *error ) const;
    void wrapSink( OSyncObjTypeSink *sink );

    OSyncObjTypeSink* sink() const { return mSink; }

  private:
    OSyncObjTypeSinkFunctions mWrapedFunctions;
    mutable OSyncContext *mContext;
    OSyncObjTypeSink *mSink;
    OSyncPluginInfo *mPluginInfo;
};

#endif
