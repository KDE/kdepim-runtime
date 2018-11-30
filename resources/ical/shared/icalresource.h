/*
    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2009 David Jarvie <djarvie@kde.org>

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

#ifndef ICALRESOURCE_H
#define ICALRESOURCE_H

#include "icalresourcebase.h"

#include <KCalCore/IncidenceBase>

class ICalResource : public ICalResourceBase
{
    Q_OBJECT

public:
    explicit ICalResource(const QString &id);
    ~ICalResource() override;

protected:
    /**
     * Constructor for derived classes.
     * @param mimeTypes mimeTypes to be handled by the resource.
     * @param icon icon name to use.
     */
    ICalResource(const QString &id, const QStringList &mimeTypes, const QString &icon);

    bool doRetrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void doRetrieveItems(const Akonadi::Collection &col) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

    /**
      Returns the Akonadi specific @c text/calendar sub MIME type of the given @p incidence.
    */
    virtual QString mimeType(const KCalCore::IncidenceBase::Ptr &incidence) const;

    /**
      Returns a list of all calendar component sub MIME types.
     */
    virtual QStringList allMimeTypes() const;
};

#endif
