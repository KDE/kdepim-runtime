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

#include "removecollectiontask.h"

#include <KDE/KDebug>
#include <KDE/KLocale>

#include <kimap/deletejob.h>
#include <kimap/session.h>

RemoveCollectionTask::RemoveCollectionTask( ResourceStateInterface::Ptr resource, QObject *parent )
  : ResourceTask( DeferIfNoSession, resource, parent )
{

}

RemoveCollectionTask::~RemoveCollectionTask()
{
}

void RemoveCollectionTask::doStart( KIMAP::Session *session )
{
  const QString mailBox = mailBoxForCollection( collection() );

  KIMAP::DeleteJob *job = new KIMAP::DeleteJob( session );
  job->setMailBox( mailBox );

  connect( job, SIGNAL(result(KJob*)),
           this, SLOT(onDeleteDone(KJob*)) );

  job->start();
}

void RemoveCollectionTask::onDeleteDone( KJob *job )
{
  // finish the task.
  changeProcessed();

  if ( job->error() ) {
    kDebug(5327) << "Failed to delete the folder, resync the folder tree";
    emitWarning( i18n( "Failed to delete the folder, restoring folder list." ) );
    synchronizeCollectionTree();
  }
}

#include "removecollectiontask.moc"


