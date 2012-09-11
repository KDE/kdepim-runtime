/*
    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef INDEXHELPERMODEL_H
#define INDEXHELPERMODEL_H

#include <akonadi/entitytreemodel.h>
#include <kurl.h>

#include <nepomukfeeder-config.h>

class IndexHelperModel: public Akonadi::EntityTreeModel
{
    Q_OBJECT
public:
    explicit IndexHelperModel(Akonadi::ChangeRecorder* monitor, QObject* parent = 0);
    virtual QVariant entityData(const Akonadi::Item& item, int column, int role = Qt::DisplayRole) const;
    virtual QVariant entityData(const Akonadi::Collection& collection, int column, int role = Qt::DisplayRole) const;
private:
    int compatLevel(Akonadi::Entity::Id id, const KUrl &url, int indexerLevel = NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL) const;
    mutable QMap<Akonadi::Entity::Id, int> mCompatLevels;
};

#endif // INDEXHELPERMODEL_H
