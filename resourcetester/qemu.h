/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef QEMU_H
#define QEMU_H

#include <QObject>

class KConfig;
class KProcess;

class QEmu : public QObject
{
  Q_OBJECT
  public:
    QEmu( QObject *parent = 0 );
    ~QEmu();

  public slots:
    void setVMConfig( const QString &configFileName );
    void start();
    void stop();
    int portOffset() const;

  private:
    QString vmImage() const;
    void waitForPort( int port );

  private:
    KConfig* mVMConfig;
    KProcess* mVMProcess;
    int mPortOffset;
    int mMonitorPort;
};

#endif
