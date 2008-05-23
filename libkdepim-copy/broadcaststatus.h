/*
  broadcaststatus.h

  This file is part of libkdepim.

  Copyright (C) 2000 Don Sanders <sanders@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KDEPIM_BROADCASTSTATUS_H
#define KDEPIM_BROADCASTSTATUS_H

#include "kdepim_export.h"
#include <QObject>

#undef None

namespace KPIM {

class ProgressItem;

/**
   Provides a singleton which broadcasts status messages by emitting
   signals. Interested mainwindows can connect to the statusMsg()
   signal and update statusBars or whatever they use for showing status.
*/
class KDEPIM_EXPORT BroadcastStatus : public QObject
{

  Q_OBJECT

  public:
    virtual ~BroadcastStatus();

    /** Return the instance of the singleton object for this class */
    static BroadcastStatus *instance();

    /** Return the last status message from setStatusMsg() */
    QString statusMsg() const { return mStatusMsg; }

    /** Sets a status bar message with timestamp */
    void setStatusMsgWithTimestamp( const QString &message );

    /** Sets a transmission completed status bar message */
    void setStatusMsgTransmissionCompleted( int numMessages,
                                            int numBytes = -1,
                                            int numBytesRead = -1,
                                            int numBytesToRead = -1,
                                            bool mLeaveOnServer = false,
                                            KPIM::ProgressItem *progressItem = 0 );
    void setStatusMsgTransmissionCompleted( const QString &account,
                                            int numMessages,
                                            int numBytes = -1,
                                            int numBytesRead = -1,
                                            int numBytesToRead = -1,
                                            bool mLeaveOnServer = false,
                                            KPIM::ProgressItem *progressItem = 0 );

  public Q_SLOTS:
    /**
      Emit an update status bar signal. It's a slot so it can be hooked up
      to other signals.
    */
    void setStatusMsg( const QString &message );

    /**
      Set a status message that will go away again with the next call of
      reset().
    */
    void setTransientStatusMsg( const QString &msg );

    /**
      Reset the status message to what ever non-transient message was last
      active or has since been set.
     */
    void reset();

  Q_SIGNALS:
    /** Emitted when setStatusMsg is called. */
    void statusMsg( const QString & );

  protected:
    BroadcastStatus();
    QString mStatusMsg;
    bool mTransientActive;
    static BroadcastStatus *instance_;
};

}

#endif
