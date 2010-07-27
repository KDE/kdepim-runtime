/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef MOVEITEMTASK_H
#define MOVEITEMTASK_H

#include "resourcetask.h"

class MoveItemTask : public ResourceTask
{
  Q_OBJECT

public:
  explicit MoveItemTask( ResourceStateInterface::Ptr resource, QObject *parent = 0 );
  virtual ~MoveItemTask();

private slots:
  void onSelectDone( KJob *job );
  void onCopyDone( KJob *job );
  void onStoreFlagsDone( KJob *job );

protected:
  virtual void doStart( KIMAP::Session *session );

private:
  void triggerCopyJob( KIMAP::Session *session );

  Akonadi::Item m_item;
};

#endif
