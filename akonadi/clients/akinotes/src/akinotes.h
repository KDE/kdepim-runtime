/*-
 * Copyright 2009 KDAB and Guillermo A. Amaral B. <gamaral@kdab.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
 
#ifndef AKINOTES_H
#define AKINOTES_H

#include <QObject>

#include <QMap>

#include <Akonadi/Entity>

namespace Akonadi { class Monitor;
		    class Item; };

class AkiNoteItem;
class KJob;

class AkiNotes : public QObject
{
Q_OBJECT

public:
	AkiNotes(QObject *_parent = 0);
	~AkiNotes(void);

private slots:
	void noteItemAdded(const Akonadi::Item &_item, const Akonadi::Collection &_collection);
	void noteItemChanged(const Akonadi::Item &_item, const QSet<QByteArray> &_partIdentifiers);
	void noteItemRemoved(const Akonadi::Item &_item);
        void noteFetchCollectionsDone(KJob* _job);
        void noteFetchDone(KJob* _job);
private:
	QMap<Akonadi::Entity::Id, AkiNoteItem *> m_items;
	Akonadi::Monitor *m_monitor;
	
};

#endif // !AKINOTES_H

