/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "notecreatorandselector.h"

#include <KLocale>

#include <KMime/Message>

#include <akonadi/item.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemcreatejob.h>

#include "note.h"
#include <Akonadi/EntityTreeModel>

using namespace Akonadi;

using namespace Akonotes;

NoteCreatorAndSelector::NoteCreatorAndSelector(QItemSelectionModel* primaryModel, QItemSelectionModel* secondaryModel, QObject* parent)
  : QObject(parent),
  m_primarySelectionModel(primaryModel),
  m_secondarySelectionModel(secondaryModel == 0 ? primaryModel : secondaryModel),
  m_containerCollectionId(-1),
  m_newNoteId(-1),
  m_giveupTimer(new QTimer(this))
{
  // If the note doesn't exist after 5 seconds, give up waiting for it.
  m_giveupTimer->setInterval(5000);
  connect(m_giveupTimer, SIGNAL(timeout()), SLOT(deleteLater()));
}

NoteCreatorAndSelector::~NoteCreatorAndSelector()
{
}


void NoteCreatorAndSelector::createNote(const Akonadi::Collection& containerCollection)
{
  m_containerCollectionId = containerCollection.id();

  if (m_primarySelectionModel == m_secondarySelectionModel)
    doCreateNote();
  else
  {
    m_giveupTimer->start();
    connect(m_primarySelectionModel->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(trySelectCollection()));
    trySelectCollection();
  }
}

void NoteCreatorAndSelector::trySelectCollection()
{
  QModelIndex idx = EntityTreeModel::modelIndexForCollection(m_primarySelectionModel->model(), Collection(m_containerCollectionId));
  if (!idx.isValid())
    return;

  m_giveupTimer->stop();
  m_primarySelectionModel->select(QItemSelection(idx, idx), QItemSelectionModel::Select);
  disconnect(m_primarySelectionModel->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(trySelectCollection()));
  doCreateNote();
}

void NoteCreatorAndSelector::doCreateNote()
{
  Item newItem;
  newItem.setMimeType( Note::mimeType() );

  KMime::Message::Ptr newPage = KMime::Message::Ptr( new KMime::Message() );

  QString title = i18nc( "The default name for new pages.", "New Page" );
  QByteArray encoding( "utf-8" );

  newPage->subject( true )->fromUnicodeString( title, encoding );
  newPage->contentType( true )->setMimeType( "text/plain" );
  newPage->date( true )->setDateTime( KDateTime::currentLocalDateTime() );
  newPage->from( true )->fromUnicodeString( QString::fromLatin1( "Kjots@kde4" ), encoding );
  // Need a non-empty body part so that the serializer regards this as a valid message.
  newPage->mainBodyPart()->fromUnicodeString( QString::fromLatin1( " " ) );

  newPage->assemble();

  newItem.setPayload( newPage );

  Akonadi::EntityDisplayAttribute *eda = new Akonadi::EntityDisplayAttribute();
  eda->setIconName( QString::fromLatin1( "text-plain" ) );
  newItem.addAttribute(eda);

  Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( newItem, Collection(m_containerCollectionId), this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT(noteCreationFinished(KJob*)) );

}

void NoteCreatorAndSelector::noteCreationFinished(KJob* job)
{
  if (job->error())
  {
    kWarning() << job->errorString();
    return;
  }
  Akonadi::ItemCreateJob *createJob = qobject_cast<Akonadi::ItemCreateJob*>(job);
  Q_ASSERT(createJob);

  Item newItem = createJob->item();
  m_newNoteId = newItem.id();

  m_giveupTimer->start();
  connect(m_primarySelectionModel->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(trySelectNote()));
  trySelectNote();
}

void NoteCreatorAndSelector::trySelectNote()
{
  QModelIndexList list = EntityTreeModel::modelIndexesForItem(m_secondarySelectionModel->model(), Item(m_newNoteId));
  if (list.isEmpty())
    return;

  const QModelIndex idx = list.first();
  m_secondarySelectionModel->select(idx, QItemSelectionModel::ClearAndSelect);
}

