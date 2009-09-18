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

#include <Akonadi/Entity>

#include <QAbstractSocket>
#include <QList>
#include <QMap>

namespace Akonadi { class Monitor;
		    class Item; };

namespace StickyNotes {	class BaseNoteItem;
			class RemoteNoteController; }

class AkiNoteItem;
class KJob;
class QTcpSocket;

class AkiNotes : public QObject
{
Q_OBJECT

public:
	enum Ui{uiStandalone, uiRemote, uiAuto};
public:
	AkiNotes(QObject *_parent = 0, Ui _ui = uiAuto);
	~AkiNotes(void);

private:
	void clearGUItems(void);
	void createAkonadiMonitor(void);
	void createGUItem(AkiNoteItem *_item);
	void createGUItems(void);
	void createRemoteController(void);
	void destroyAkonadiMonitor(void);
	void destroyRemoteController(void);

private slots:
	void on_monitor_itemAdded(const Akonadi::Item &_item, const Akonadi::Collection &_collection);
	void on_monitor_itemChanged(const Akonadi::Item &_item, const QSet<QByteArray> &_partIdentifiers);
	void on_monitor_itemRemoved(const Akonadi::Item &_item);
	void on_remoteController_fallback(void);
	void on_remoteSocket_connected(void);
	void on_remoteSocket_disconnected(void);
	void on_remoteSocket_error(QAbstractSocket::SocketError _socketError);
        void on_fetchCollectionsJob_done(KJob* _job);
        void on_fetchItemJob_done(KJob* _job);

private:
	QList<StickyNotes::BaseNoteItem *> m_guitems;
	QMap<Akonadi::Entity::Id, AkiNoteItem *> m_items;
	Akonadi::Monitor *m_monitor;
	StickyNotes::RemoteNoteController *m_remoteController;
	QTcpSocket *m_remoteSocket;
	Ui   m_ui;
};

#endif // !AKINOTES_H

