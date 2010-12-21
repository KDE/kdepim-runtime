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

#ifndef RETRIEVECOLLECTIONSTASK_H
#define RETRIEVECOLLECTIONSTASK_H

#include <akonadi/collection.h>

#include <kimap/listjob.h>

#include "resourcetask.h"

class RetrieveCollectionsTask : public ResourceTask
{
  Q_OBJECT

public:
  explicit RetrieveCollectionsTask( ResourceStateInterface::Ptr resource, QObject *parent = 0 );
  virtual ~RetrieveCollectionsTask();

private slots:
  void onMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &descriptors,
                            const QList< QList<QByteArray> > &flags );
  void onMailBoxesReceiveDone( KJob *job );
  void onFullMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &descriptors, const QList<QList<QByteArray> > &flags );
  void onFullMailBoxesReceiveDone( KJob *job );

protected:
  virtual void doStart( KIMAP::Session *session );

private:
  QHash<QString, Akonadi::Collection> m_reportedCollections;
  QHash<QString, Akonadi::Collection> m_dummyCollections;
  QSet<QString> m_fullReportedCollections;
};

#endif
