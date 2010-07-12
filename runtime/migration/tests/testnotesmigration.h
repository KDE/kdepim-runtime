/*
    Copyright 2010 Stephen Kelly <steveire@gmail.com>

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

#ifndef NOTESMIGRATIONTEST_H
#define NOTESMIGRATIONTEST_H

#include <QtCore/QObject>

#include <Akonadi/Collection>
#include <akonadi/qtest_akonadi.h>
#include <QtTest/qtestcase.h>

namespace Akonadi
{
class EntityTreeModel;
}

using namespace Akonadi;

class NotesMigrationTest : public QObject
{
  Q_OBJECT

public:
  NotesMigrationTest(QObject* parent = 0)
    : QObject(parent)
  {}

protected Q_SLOTS:
  void checkRowsInserted( const QModelIndex &parent, int start, int end );

private Q_SLOTS:
  void initTestCase();
  void testKJotsBooksMigration();

  void testLocalKNotesMigration();

private:
  Akonadi::EntityTreeModel *m_etm;

  // Use by kjots.
  QHash<QString, QStringList> m_expectedStructure;
  QHash<QString, QStringList> m_seenStructure;

  // Used by notes.
  QStringList m_expectedNotes;
  QStringList m_seenNotes;


};

#endif
