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


#include "indexhelpermodel.h"
#include <aneo.h>
#include <nie.h>
#include <KUrl>
#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Nepomuk2/ResourceManager>

IndexHelperModel::IndexHelperModel(Akonadi::ChangeRecorder* monitor, QObject* parent): EntityTreeModel(monitor, parent)
{
}

int IndexHelperModel::compatLevel(Akonadi::Entity::Id id, const KUrl &url, int indexerLevel) const
{
    if (!mCompatLevels.contains(id)) {
        Soprano::QueryResultIterator result = Nepomuk2::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("select ?compatLevel where { ?r %1 %2 . ?r %3 ?compatLevel . }")
                .arg(Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                Soprano::Node::resourceToN3(url),
                Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiIndexCompatLevel())),
                Soprano::Query::QueryLanguageSparql);
        Q_ASSERT(result.isValid());
        if (result.next()) {
            mCompatLevels.insert(id, result[0].literal().toInt());
        } else {
            mCompatLevels.insert(id, -1);
        }
    }
    return mCompatLevels.value(id);
}

static QString getIndexedLevelString(int compatLevel)
{
    if (compatLevel > 0) {
        return QString::fromLatin1(" [IndexingLevel: %1]").arg(compatLevel);
    }
    if (compatLevel == -1) {
        return QLatin1String("[Not Indexed]");
    }
    return QLatin1String("[No Info]");
}

QVariant IndexHelperModel::entityData(const Akonadi::Item& item, int column, int role) const
{
    switch(role) {
        case Qt::DisplayRole: {
            switch (column) {
                case 0: {
                    QString s = Akonadi::EntityTreeModel::entityData(item, column, role).toString();
                    int compat = compatLevel(item.id(),item.url());
                    s.append(getIndexedLevelString(compat));
                    return s;
                }
            }
        }
        break;
        case Qt::ToolTipRole: {
            QString d;
            d.append(QString::fromLatin1("Akonadi: %1\n").arg(item.url().url()));
            d.append(getIndexedLevelString(compatLevel(item.id(),item.url())));
            return d;
        }
    }
    return Akonadi::EntityTreeModel::entityData(item, column, role);
}

QVariant IndexHelperModel::entityData(const Akonadi::Collection& collection, int column, int role) const
{
    switch(role) {
        case Qt::DisplayRole: {
            switch (column) {
                case 0:
                    return Akonadi::EntityTreeModel::entityData(collection, column, role).toString().append(getIndexedLevelString(compatLevel(collection.id(), collection.url())));
            }
        }
    }
    return Akonadi::EntityTreeModel::entityData(collection, column, role);
}


#include "indexhelpermodel.moc"
