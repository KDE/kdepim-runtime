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
 
#include "akinotes.h"

#include <QStringList>

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>

#include <StickyNotes/StandaloneNoteItem>

#include "akinoteitem.h"

#include <KDebug>

AkiNotes::AkiNotes(QObject *_parent)
: QObject(_parent), m_monitor(new Akonadi::Monitor(this))
{
	m_monitor->setMimeTypeMonitored("application/x-vnd.kde.notes");
	m_monitor->setCollectionMonitored(Akonadi::Collection::root(), false);
	m_monitor->itemFetchScope().fetchFullPayload();

	connect(m_monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)),
	    this, SLOT(noteItemAdded(Akonadi::Item)));

	connect(m_monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)),
	    this, SLOT(noteItemChanged(Akonadi::Item)));

        Akonadi::Collection noteCollection(Akonadi::Collection::root());
        noteCollection.setContentMimeTypes(QStringList() << "application/x-vnd.kde.notes");
        Akonadi::CollectionFetchJob *fetch =
	    new Akonadi::CollectionFetchJob(noteCollection, Akonadi::CollectionFetchJob::Recursive);
        connect(fetch, SIGNAL(result(KJob*)), SLOT(fetchNoteCollectionsDone(KJob*)));
}

AkiNotes::~AkiNotes(void)
{
	disconnect(m_monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)),
	    this, SLOT(noteItemAdded(Akonadi::Item)));

	foreach(AkiNoteItem *item, m_items.values())
		delete item;

	m_items.clear();
}

void
AkiNotes::noteItemAdded(const Akonadi::Item &_item)
{
	AkiNoteItem *item = new AkiNoteItem(_item, *this);

	if (!item->isValid()) {
	    delete item;
	    return;
	}

	m_monitor->setItemMonitored(_item);

	m_items[_item.id()] = item;
#ifdef LSN_PLASMA_GUI_AVAILABLE
	// replace with remote item
	new StickyNotes::StandaloneNoteItem(item);
#elif LSN_STANDALONE_GUI_AVAILABLE
	new StickyNotes::StandaloneNoteItem(item);
#else
	kDebug() << "No GUI Available";
#endif
}

void
AkiNotes::noteItemChanged(const Akonadi::Item &_item)
{
}

void
AkiNotes::fetchNoteCollectionsDone(KJob* _job)
{
	if (_job->error() ) {
		kDebug() << "Job Error:" << _job->errorString();
		return;
	}

	Akonadi::CollectionFetchJob* cjob = static_cast<Akonadi::CollectionFetchJob*>(_job);
	foreach(const Akonadi::Collection &collection, cjob->collections()) {
		if (collection.contentMimeTypes().contains("application/x-vnd.kde.notes")) {
			kDebug() << "Note res found:" << collection.name();

			Akonadi::ItemFetchJob *fetch = new Akonadi::ItemFetchJob(collection);
			m_monitor->setCollectionMonitored(collection, true);
			fetch->fetchScope().fetchFullPayload();
			connect(fetch, SIGNAL(result(KJob*)), SLOT(fetchNoteDone(KJob*)));
		}
	}
}

void
AkiNotes::fetchNoteDone(KJob* _job)
{
	if (_job->error()) {
		kDebug() << "Note job failed:" << _job->errorString();
		return;
	}

	Akonadi::Item::List items = static_cast<Akonadi::ItemFetchJob*>(_job)->items();
	kDebug() << "Adding notes" << items.count();

	foreach (const Akonadi::Item &item, items) 
		noteItemAdded(item);
}

