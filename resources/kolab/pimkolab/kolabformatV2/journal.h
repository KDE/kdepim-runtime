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

#ifndef KOLABV2_JOURNAL_H
#define KOLABV2_JOURNAL_H

#include <kcalcore/journal.h>

#include "kolabbase.h"

class QDomElement;

namespace KolabV2 {
/**
 * This class represents a journal entry, and knows how to load/save it
 * from/to XML, and from/to a KCalCore::Journal.
 * The instances of this class are temporary, only used to convert
 * one to the other.
 */
class Journal : public KolabBase
{
public:
    /// Use this to parse an xml string to a journal entry
    /// The caller is responsible for deleting the returned journal
    static KCalCore::Journal::Ptr fromXml(const QDomDocument &xmlDoc, const QString &tz);

    /// Use this to get an xml string describing this journal entry
    static QString journalToXML(const KCalCore::Journal::Ptr &, const QString &tz);

    explicit Journal(const QString &tz, const KCalCore::Journal::Ptr &journal = KCalCore::Journal::Ptr());
    ~Journal() override;

    QString type() const override
    {
        return QStringLiteral("Journal");
    }

    void saveTo(const KCalCore::Journal::Ptr &journal) const;

    virtual void setSummary(const QString &summary);
    virtual QString summary() const;

    virtual void setStartDate(const QDateTime &startDate);
    virtual QDateTime startDate() const;

    virtual void setEndDate(const QDateTime &endDate);
    virtual QDateTime endDate() const;

    // Load the attributes of this class
    bool loadAttribute(QDomElement &) override;

    // Save the attributes of this class
    bool saveAttributes(QDomElement &) const override;

    // Load this journal by reading the XML file
    bool loadXML(const QDomDocument &xml) override;

    // Serialize this journal to an XML string
    QString saveXML() const override;

protected:
    // Read all known fields from this ical journal
    void setFields(const KCalCore::Journal::Ptr &);

    QString productID() const override;

    QString mSummary;
    QDateTime mStartDate;
    QDateTime mEndDate;
    bool mDateOnly = false;
};
}

#endif // KOLAB_JOURNAL_H
