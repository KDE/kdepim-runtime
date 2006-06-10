/*
    This file is part of libkdepim.

    Copyright (c) 2005 Tobias Koenig <tokoe@kde.org>

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
*/

#ifndef NETWORKSTATUS_H
#define NETWORKSTATUS_H

#include <QObject>

namespace KPIM {

/**
    This is a class for monitoring network status -- basically,
    the machine KDE is running on going from "online" mode to
    offline. What this means is left as an exercise for the reader.
 */
class NetworkStatus : public QObject
{
  Q_OBJECT

  public:
    /**
     * The possible states.
     */
    enum Status {
      Online,     //< The machine now has internet connectivity
      Offline     //< The machine has no internet connectivity
    };

    /**
     * Destructor.
     */
    ~NetworkStatus();

    /**
     * Returns the only instance of this class.
     */
    static NetworkStatus *self();

    /**
     * Sets a new status.
     *
     * @param status The new status.
     */
    void setStatus( Status status );

    /**
     * Returns the current status.
     */
    Status status() const;

protected slots:
    /**
     * Called by the network interface watcher in KDED.
     */
    void onlineStatusChanged();

  signals:
    /**
     * Emitted whenever the status has changed.
     *
     * @param status The new status.
     */
    void statusChanged( Status status );

  protected:
    /**
     * Constructor. This is protected, so you must use self()
     * to get the singleton object of this class.
     */
    NetworkStatus();

  private:
    Status mStatus;
    static NetworkStatus *mSelf;
};

}

#endif
