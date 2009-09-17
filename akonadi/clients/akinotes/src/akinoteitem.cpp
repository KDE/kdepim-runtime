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
 
#include "akinoteitem.h"

#include <QByteArray>

#include <Akonadi/ItemModifyJob>
#include <KCal/Incidence>


#include <KDebug>

using namespace StickyNotes;

/* AkiNoteItem */

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

AkiNoteItem::AkiNoteItem(const Akonadi::Item &_item, AkiNotes &_owner)
: BaseNoteItem(0), m_item(_item), m_owner(_owner), m_valid(false)
{
	if (m_item.hasPayload<IncidencePtr>()) {
		m_incidence = m_item.payload<IncidencePtr>();
		m_valid = true;
	}
}

AkiNoteItem::~AkiNoteItem(void)
{
}

bool
AkiNoteItem::isValid(void) const
{
	return (m_valid);
}

QVariant
AkiNoteItem::attribute(const QString &_name) const
{
	return (m_incidence->nonKDECustomProperty(_name.toAscii()));
}

QList<QString>
AkiNoteItem::attributeNames(void) const
{
	QList<QString> keys;

	foreach(QByteArray key, m_incidence->customProperties().keys())
		keys << key;

	return (keys);
}

QString
AkiNoteItem::content(void) const
{
	return (m_incidence->description());
}

QString
AkiNoteItem::subject(void) const
{
	return (m_incidence->summary());
}

bool
AkiNoteItem::applyAttribute(BaseNoteItem * const _sender,
    const QString &_name, const QVariant &_value)
{
	if (attribute(_name) == _value)
		return false;

	m_incidence->setNonKDECustomProperty(_name.toAscii(), _value.toString());
	m_item.setPayload<IncidencePtr>(m_incidence);

	Akonadi::ItemModifyJob* sJob = new Akonadi::ItemModifyJob(m_item);
	if (!sJob->exec()) {
		kDebug() << sJob->errorString();
		return (false);
	}

	return (true);
}

bool
AkiNoteItem::applyContent(BaseNoteItem * const _sender,
    const QString &_content)
{
	if (0 == content().compare(_content))
		return (false);

	m_incidence->setDescription(_content);
	m_item.setPayload<IncidencePtr>(m_incidence);

	Akonadi::ItemModifyJob* sJob = new Akonadi::ItemModifyJob(m_item);
	if (!sJob->exec()) {
		kDebug() << sJob->errorString();
		return (false);
	}

	return (true);
}

bool
AkiNoteItem::applySubject(BaseNoteItem * const _sender,
    const QString &_subject)
{
	if (0 == subject().compare(_subject))
		return (false);

	m_incidence->setSummary(_subject);
	m_item.setPayload<IncidencePtr>(m_incidence);

	Akonadi::ItemModifyJob* sJob = new Akonadi::ItemModifyJob(m_item);
	if (!sJob->exec()) {
		kDebug() << sJob->errorString();
		return (false);
	}

	return (true);
}

