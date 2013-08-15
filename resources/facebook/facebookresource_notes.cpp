/*
  Copyright 2010, 2011 Thomas McGuire <mcguire@kde.org>
  Copyright 2011 Roeland Jago Douma <unix@rullzer.com>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published
  by the Free Software Foundation; either version 2 of the License or
  ( at your option ) version 3 or, at the discretion of KDE e.V.
  ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "facebookresource.h"
#include "settings.h"
#include "settingsdialog.h"
#include "timestampattribute.h"

#include <libkfbapi/allnoteslistjob.h>
#include <libkfbapi/notejob.h>
#include <libkfbapi/noteaddjob.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>

using namespace Akonadi;

void FacebookResource::noteListFetched( KJob *job )
{
  Q_ASSERT( !mIdle );
  KFbAPI::AllNotesListJob * const listJob = dynamic_cast<KFbAPI::AllNotesListJob*>( job );
  Q_ASSERT( listJob );
  mCurrentJobs.removeAll( job );

  if ( listJob->error() ) {
    abortWithError( i18n( "Unable to get notes from server: %1", listJob->errorString() ),
                    listJob->error() == KFbAPI::FacebookJob::AuthenticationProblem );
  } else {
    setItemStreamingEnabled( true );

    Item::List noteItems;
    foreach ( const KFbAPI::NoteInfo &noteInfo, listJob->allNotes() ) {
      Item note;
      note.setRemoteId( noteInfo.id() );
      note.setPayload<KMime::Message::Ptr>( noteInfo.asNote() );
      note.setSize( noteInfo.asNote()->size() );
      note.setMimeType( QLatin1String("text/x-vnd.akonadi.note") );
      noteItems.append( note );
    }

    itemsRetrieved( noteItems );
    itemsRetrievalDone();
    finishNotesFetching();
  }

  listJob->deleteLater();
}

void FacebookResource::finishNotesFetching()
{
  emit percent( 100 );
  emit status( Idle, i18n( "All notes fetched from server." ) );
  resetState();
}

void FacebookResource::noteJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::NoteJob * const noteJob = dynamic_cast<KFbAPI::NoteJob*>( job );
  Q_ASSERT( noteJob );
  Q_ASSERT( noteJob->noteInfo().size() == 1 );
  mCurrentJobs.removeAll( job );

  if ( noteJob->error() ) {
    abortWithError( i18n( "Unable to get information about note from server: %1",
                          noteJob->errorText() ) );
  } else {
    Item note = noteJob->property( "Item" ).value<Item>();
    note.setPayload( noteJob->noteInfo().first().asNote() );
    itemRetrieved( note );
    resetState();
  }

  noteJob->deleteLater();
}

void FacebookResource::noteAddJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::NoteAddJob * const addJob = dynamic_cast<KFbAPI::NoteAddJob*>( job );
  Q_ASSERT( addJob );
  mCurrentJobs.removeAll( job );

  if ( job->error() ) {
    abortWithError( i18n( "Unable to get upload note to server: %1", job->errorText() ) );
  } else {
    Item note = addJob->property( "Item" ).value<Item>();
    note.setRemoteId( addJob->property( "id" ).value<QString>() );
    changeCommitted( note );
    resetState();
  }

  addJob->deleteLater();
}
