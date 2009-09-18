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
#include <QTcpSocket>

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>

#include <StickyNotes/RemoteNoteController>
#include <StickyNotes/RemoteNoteItem>
#include <StickyNotes/StandaloneNoteItem>

#include "akinoteitem.h"

#include <KDebug>

AkiNotes::AkiNotes(QObject *_parent, AkiNotes::Ui _ui)
: QObject(_parent), m_monitor(0), m_remoteController(0), m_remoteSocket(0),
    m_ui(_ui)
{
	createAkonadiMonitor();

	if (uiStandalone != m_ui)
		createRemoteController();
}

AkiNotes::~AkiNotes(void)
{
	if (uiStandalone != m_ui)
		destroyRemoteController();

	destroyAkonadiMonitor();
}

void
AkiNotes::clearGUItems(void)
{
	foreach(StickyNotes::BaseNoteItem *guitem, m_guitems)
		delete guitem;

	m_guitems.clear();
}

void
AkiNotes::createAkonadiMonitor(void)
{
	m_monitor = new Akonadi::Monitor(this);

	Akonadi::CollectionFetchJob *fetch;

	connect(m_monitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)),
	    this, SLOT(on_monitor_itemAdded(const Akonadi::Item &, const Akonadi::Collection &)));
	connect(m_monitor, SIGNAL(itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)),
	    this, SLOT(on_monitor_itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)));
	connect(m_monitor, SIGNAL(itemRemoved(const Akonadi::Item &)),
	    this, SLOT(on_monitor_itemRemoved(const Akonadi::Item &)));

	m_monitor->setMimeTypeMonitored("application/x-vnd.kde.notes");
	m_monitor->setCollectionMonitored(Akonadi::Collection::root(), false);
	m_monitor->itemFetchScope().fetchFullPayload();

	// Fetch collections
	Akonadi::Collection noteCollection(Akonadi::Collection::root());
	noteCollection.setContentMimeTypes(QStringList() << "application/x-vnd.kde.notes");
	fetch = new Akonadi::CollectionFetchJob(noteCollection,
	    Akonadi::CollectionFetchJob::Recursive);
        connect(fetch, SIGNAL(result(KJob*)),
	    this, SLOT(on_fetchCollectionsJob_done(KJob*)));
}

void
AkiNotes::createGUItem(AkiNoteItem *_item)
{
	StickyNotes::BaseNoteItem *guitem;

	switch (m_ui) {
		case uiRemote:
			guitem = m_remoteController->createItem();
			guitem->setParent(_item);
		break;
		case uiStandalone:
			guitem = new StickyNotes::StandaloneNoteItem(_item);
		break;
		default:
			kDebug() << "No ui selected";
			return;
		break;
	}
}

void
AkiNotes::createGUItems(void)
{
	kDebug() << "createGUItems";

	if (!m_guitems.isEmpty())
		clearGUItems();

	foreach(AkiNoteItem *item, m_items.values())
		createGUItem(item);
}

void
AkiNotes::createRemoteController(void)
{
	m_remoteSocket = new QTcpSocket(this);
	m_remoteController = new StickyNotes::RemoteNoteController(*m_remoteSocket);

	connect(m_remoteSocket, SIGNAL(connected()),
	    this, SLOT(on_remoteSocket_connected()));
	connect(m_remoteSocket, SIGNAL(disconnected()),
	    this, SLOT(on_remoteSocket_disconnected()));
	connect(m_remoteSocket, SIGNAL(error(QAbstractSocket::SocketError)),
	    this, SLOT(on_remoteSocket_error(QAbstractSocket::SocketError)));
	
	// TODO: make host and port configurable
	m_remoteSocket->connectToHost("127.0.0.1", 12345);
}

void
AkiNotes::destroyAkonadiMonitor(void)
{
	foreach(AkiNoteItem *item, m_items.values())
		delete item;

	m_items.clear();

	disconnect(m_monitor, SIGNAL(itemRemoved(const Akonadi::Item &)),
	    this, SLOT(on_monitor_itemRemoved(const Akonadi::Item &)));
	disconnect(m_monitor, SIGNAL(itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)),
	    this, SLOT(on_monitor_itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)));
	disconnect(m_monitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)),
	    this, SLOT(on_monitor_itemAdded(const Akonadi::Item &, const Akonadi::Collection &)));

	delete m_monitor;
}

void
AkiNotes::destroyRemoteController(void)
{
	m_remoteController->stopListening();
	delete m_remoteController;

	connect(m_remoteSocket, SIGNAL(error(QAbstractSocket::SocketError)),
	    this, SLOT(on_remoteSocket_error(QAbstractSocket::SocketError)));
	connect(m_remoteSocket, SIGNAL(disconnected()),
	    this, SLOT(on_remoteSocket_disconnected()));
	connect(m_remoteSocket, SIGNAL(connected()),
	    this, SLOT(on_remoteSocket_connected()));

	delete m_remoteSocket;
}

void
AkiNotes::on_monitor_itemAdded(const Akonadi::Item &_item, const Akonadi::Collection &_collection)
{
	Q_UNUSED(_collection);

	kDebug() << "Add note " << _item.id();
	AkiNoteItem *item = new AkiNoteItem(&_item);

	if (!item->isValid()) {
	    kDebug() << "Invalid note " << _item.id();
	    delete item;
	    return;
	}

	m_monitor->setItemMonitored(_item);

	m_items[_item.id()] = item;

	createGUItem(item);
}

void
AkiNotes::on_monitor_itemChanged(const Akonadi::Item &_item, const QSet<QByteArray> &_partIdentifiers)
{
	Q_UNUSED(_partIdentifiers);

	kDebug() << "Update note " << _item.id();
	if (m_items.contains(_item.id())) {
		m_items[_item.id()]->assign(_item);

		if (!m_items[_item.id()]->isValid()) {
			delete  m_items[_item.id()];
			m_items.remove(_item.id());
		}
	}
}

void
AkiNotes::on_monitor_itemRemoved(const Akonadi::Item &_item)
{
	kDebug() << "Remove note " << _item.id();
	if (m_items.contains(_item.id())) {
		delete  m_items[_item.id()];
		m_items.remove(_item.id());
	}
}

void
AkiNotes::on_remoteSocket_connected(void)
{
	m_remoteController->startListening();
	m_ui = uiRemote;
	createGUItems();
}

void
AkiNotes::on_remoteSocket_disconnected(void)
{
	/* TODO: More smarts needed
	 * Should we retry or fallback to standalone
	 */
	destroyRemoteController();
	m_ui = uiStandalone;
	createGUItems();
}

void
AkiNotes::on_remoteSocket_error(QAbstractSocket::SocketError _socketError)
{
	/* TODO: More smarts needed
	 * Should we retry or fallback to standalone
	 */
	destroyRemoteController();
	m_ui = uiStandalone;
	createGUItems();
}

void
AkiNotes::on_fetchCollectionsJob_done(KJob* _job)
{
	if (_job->error() ) {
		kDebug() << "Job Error:" << _job->errorString();
		return;
	}

	Akonadi::CollectionFetchJob* cjob = static_cast<Akonadi::CollectionFetchJob*>(_job);
	foreach(const Akonadi::Collection &collection, cjob->collections()) {
		if (collection.contentMimeTypes().contains("application/x-vnd.kde.notes")) {
			kDebug() << "Note collection found:" << collection.name();
			Akonadi::ItemFetchJob *fetch = new Akonadi::ItemFetchJob(collection);
			m_monitor->setCollectionMonitored(collection, true);
			fetch->fetchScope().fetchFullPayload();
			connect(fetch, SIGNAL(result(KJob*)), SLOT(on_fetchItemJob_done(KJob*)));
		}
	}
}

void
AkiNotes::on_fetchItemJob_done(KJob* _job)
{
	if (_job->error()) {
		kDebug() << "Note job failed:" << _job->errorString();
		return;
	}

	Akonadi::Item::List items = static_cast<Akonadi::ItemFetchJob*>(_job)->items();

	kDebug() << "Adding notes" << items.count();

	foreach (const Akonadi::Item &item, items) 
		on_monitor_itemAdded(item, Akonadi::Collection::root());
}

