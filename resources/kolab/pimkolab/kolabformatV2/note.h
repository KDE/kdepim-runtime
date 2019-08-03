/*
    This file is part of the kolab resource - the implementation of the
    Kolab storage format. See www.kolab.org for documentation on this.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef KOLABV2_NOTE_H
#define KOLABV2_NOTE_H

#include <kcalendarcore/journal.h>

#include "kolabbase.h"

class QDomElement;

namespace KolabV2 {
/**
 * This class represents a note, and knows how to load/save it
 * from/to XML, and from/to a KCalendarCore::Journal.
 * The instances of this class are temporary, only used to convert
 * one to the other.
 */
class Note : public KolabBase
{
public:
    /// Use this to parse an xml string to a journal entry
    /// The caller is responsible for deleting the returned journal
    static KCalendarCore::Journal::Ptr xmlToJournal(const QString &xml);

    /// Use this to get an xml string describing this journal entry
    static QString journalToXML(const KCalendarCore::Journal::Ptr &);

    /// Create a note object and
    explicit Note(const KCalendarCore::Journal::Ptr &journal = KCalendarCore::Journal::Ptr());
    virtual ~Note();

    void saveTo(const KCalendarCore::Journal::Ptr &journal) const;

    QString type() const override
    {
        return QStringLiteral("Note");
    }

    virtual void setSummary(const QString &summary);
    virtual QString summary() const;

    virtual void setBackgroundColor(const QColor &bgColor);
    virtual QColor backgroundColor() const;

    virtual void setForegroundColor(const QColor &fgColor);
    virtual QColor foregroundColor() const;

    virtual void setRichText(bool richText);
    virtual bool richText() const;

    // Load the attributes of this class
    bool loadAttribute(QDomElement &) override;

    // Save the attributes of this class
    bool saveAttributes(QDomElement &) const override;

    // Load this note by reading the XML file
    bool loadXML(const QDomDocument &xml) override;

    // Serialize this note to an XML string
    QString saveXML() const override;

protected:
    // Read all known fields from this ical incidence
    void setFields(const KCalendarCore::Journal::Ptr &);

    // Save all known fields into this ical incidence
    void saveTo(const KCalendarCore::Incidence::Ptr &) const;

    QString productID() const override;

    QString mSummary;
    QColor mBackgroundColor;
    QColor mForegroundColor;
    bool mRichText;
};
}

#endif // KOLAB_NOTE_H
