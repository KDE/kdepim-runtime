/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SIMPLERESOURCEGRAPH_H
#define SIMPLERESOURCEGRAPH_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>

#include <KGlobal>

#include "nepomukdatamanagement_export.h"

class QDataStream;

class KJob;
namespace Soprano {
class Statement;
class Node;
}
namespace Nepomuk {
class SimpleResource;
class StoreResourcesJob;

class NEPOMUK_DATA_MANAGEMENT_EXPORT SimpleResourceGraph
{
public:
    SimpleResourceGraph();
    SimpleResourceGraph(const SimpleResource& resource);
    SimpleResourceGraph(const QList<SimpleResource>& resources);
    SimpleResourceGraph(const QSet<SimpleResource>& resources);
    SimpleResourceGraph(const SimpleResourceGraph& other);
    ~SimpleResourceGraph();

    SimpleResourceGraph& operator=(const SimpleResourceGraph& other);

    /**
     * Adds a resource to the graph. An invalid resource will get a
     * new blank node as resource URI.
     */
    void insert(const SimpleResource& res);
    SimpleResourceGraph& operator<<(const SimpleResource& res);

    void remove(const QUrl& uri);
    void remove(const SimpleResource& res);

    void add(const QUrl& uri, const QUrl& property, const QVariant& value);
    void set(const QUrl& uri, const QUrl& property, const QVariant& value);

    void remove(const QUrl& uri, const QUrl& property, const QVariant& value);

    /**
     * Remove all properties matching the provided parameters. It is possible to
     * provide empty values to act as wildcards.
     */
    void removeAll(const QUrl& uri, const QUrl& property, const QVariant& value = QVariant());

    void clear();

    int count() const;
    int size() const { return count(); }
    bool isEmpty() const;

    bool contains(const SimpleResource& res) const;
    bool contains(const QUrl& res) const;
    bool containsAny(const QUrl& res, const QUrl& property) const;

    SimpleResource operator[](const QUrl& uri) const;

    /**
     * Get a list of the URIs of all resources in this graph.
     * The result is equivalent to:
     *
     * \code
     * QList<QUrl> uris;
     * foreach(const SimpleResource& res, toList())
     *     uris << res.uri();
     * \endcode
     *
     * \return A list of all resource URIs in this graph.
     */
    QList<QUrl> allResourceUris() const;

    QSet<SimpleResource> toSet() const;
    QList<SimpleResource> toList() const;

    void addStatement(const Soprano::Statement& statement);
    void addStatement(const Soprano::Node & subject,
                      const Soprano::Node & predicate,
                      const Soprano::Node & object);

    SimpleResourceGraph& operator+=( const SimpleResourceGraph & graph );

    /**
     * Save the graph to the Nepomuk database.
     * \return A job that will perform the saving
     * and emit the result() signal once done.
     * Use the typical KJob error handling methods.
     *
     * \param component The component which should be given to
     * Nepomuk for it to relate the newly created data to it.
     *
     * \sa Nepomuk::storeResources()
     */
    StoreResourcesJob* save(const KComponentData& component = KGlobal::mainComponent()) const;

    bool operator==( const SimpleResourceGraph & rhs) const;
    bool operator!=( const SimpleResourceGraph & rhs) const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};


NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator<<(QDataStream &, const Nepomuk::SimpleResourceGraph& graph);
NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator>>(QDataStream &, Nepomuk::SimpleResourceGraph& graph);
NEPOMUK_DATA_MANAGEMENT_EXPORT QDebug operator<<(QDebug dbg, const Nepomuk::SimpleResourceGraph& graph);
NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator<<(QDataStream &, const Nepomuk::SimpleResourceGraph& graph);
NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator>>(QDataStream &, Nepomuk::SimpleResourceGraph& graph);
}

#endif
