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

#ifndef NOTECREATORANDSELECTOR_H
#define NOTECREATORANDSELECTOR_H

#include <QItemSelectionModel>
#include <QTimer>

#include <akonadi/collection.h>

#include "akonadi_next_export.h"

class KJob;

namespace Akonotes
{

/**
 * @brief Creates and selects the newly created note.
 *
 * The note is created in the supplied collection. That collection is
 * selected in the primaryModel. The new note is selected in the
 * secondaryModel. If the secondaryModel is null, the primaryModel is
 * used to select the note too. That is relevant for mixed tree models like
 * KJots uses.
 */
class AKONADI_NEXT_EXPORT NoteCreatorAndSelector : public QObject
{
  Q_OBJECT
public:
  NoteCreatorAndSelector(QItemSelectionModel *primaryModel, QItemSelectionModel *secondaryModel = 0, QObject* parent = 0);

  virtual ~NoteCreatorAndSelector();

  void createNote(const Akonadi::Collection &containerCollection);

private:
  void doCreateNote();

private slots:
  void trySelectCollection();
  void noteCreationFinished( KJob *job );
  void trySelectNote();

private:
  QItemSelectionModel *m_primarySelectionModel;
  QItemSelectionModel *m_secondarySelectionModel;

  Akonadi::Entity::Id m_containerCollectionId;
  Akonadi::Entity::Id m_newNoteId;
  QTimer *m_giveupTimer;

};

}

#endif
