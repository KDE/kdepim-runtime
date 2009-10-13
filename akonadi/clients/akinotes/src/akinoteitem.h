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
 
#ifndef AKINOTEITEM_H
#define AKINOTEITEM_H

#include <Akonadi/Item>
#include <StickyNotes/BaseNoteItem>
#include <boost/shared_ptr.hpp>

namespace KCal { class Incidence; }

class AkiNotes;

class AkiNoteItem : public StickyNotes::BaseNoteItem
{
public:
	explicit AkiNoteItem(const Akonadi::Item *_item = 0);
	virtual ~AkiNoteItem(void);

	void assign(const Akonadi::Item &_item);
	bool isValid(void) const;

	/* virtual */

	virtual QVariant attribute(const QString &_name) const;
	virtual QList<QString> attributeNames(void) const;
	virtual QString  content(void) const;
	virtual QString  subject(void) const;

protected:
	virtual bool applyAttribute(BaseNoteItem * const _sender,
	    const QString &_name, const QVariant &_value);
	virtual bool applyContent(BaseNoteItem * const _sender,
	    const QString &_content);
	virtual bool applySubject(BaseNoteItem * const _sender,
	    const QString &_subject);

private:
	boost::shared_ptr<KCal::Incidence> m_incidence;
	Akonadi::Item m_item;
	bool m_valid;
};

#endif // !AKINOTEITEM_H

