/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2010-2011 Vishesh Handa <handa.vish@gmail.com>

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

#ifndef SIMPLERESOURCE_H
#define SIMPLERESOURCE_H

#include <QtCore/QUrl>
#include <QtCore/QMultiHash>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>

#include <Soprano/Statement>

#include "nepomukdatamanagement_export.h"

namespace Nepomuk {

typedef QMultiHash<QUrl, QVariant> PropertyHash;

/**
 * \class SimpleResource simpleresource.h Nepomuk/SimpleResource
 *
 * \brief Represents a snapshot of one %Nepomuk resource.
 *
 * \author Vishesh Handa <handa.vish@gmail.com>, Sebastian Trueg <trueg@kde.org>
 */
class NEPOMUK_DATA_MANAGEMENT_EXPORT SimpleResource
{
public:
    explicit SimpleResource(const QUrl& uri = QUrl());
    SimpleResource(const PropertyHash& properties);
    SimpleResource(const SimpleResource& other);
    virtual ~SimpleResource();

    SimpleResource& operator=(const SimpleResource& other);

    bool operator==(const SimpleResource& other) const;

    QUrl uri() const;

    /**
     * Setting an invalid/empty uri will create a new random ID.
     * This allows to reuse SimpleResource instances.
     */
    void setUri( const QUrl & uri );

    bool contains(const QUrl& property) const;
    bool contains(const QUrl& property, const QVariant& value) const;
    bool containsNode(const QUrl& property, const Soprano::Node& value) const;

    /**
     * Clear the resource, remove all properties.
     */
    void clear();

    /**
     * Set the properties, replacing the existing properties.
     */
    void setProperties(const PropertyHash& properties);

    /**
     * Add a set of properties to the existing ones.
     */
    void addProperties(const PropertyHash& properties);

    /**
     * Set a property overwriting existing values.
     * \param property The property to set
     * \param value The value of the property.
     */
    void setProperty(const QUrl& property, const QVariant& value);

    /**
     * Set a property overwriting existing values.
     * \param property The property to set
     * \param values The values of the property.
     */
    void setProperty(const QUrl& property, const QVariantList& values);

    /**
     * Set a property overwriting existing values.
     * \param property The property to set
     * \param set The values of the property.
     */
    void setProperty(const QUrl& property, const SimpleResource& res);

    /**
     * Set a property overwriting existing values.
     * \param property The property to set
     * \param value The value of the property. Will be converted to a QVariant.
     */
    void setPropertyNode(const QUrl& property, const Soprano::Node& value);

    /**
     * Add a property. This allows to add more than one value for a property.
     * \param property The property to set
     * \param value The value of the property.
     */
    void addProperty(const QUrl& property, const QVariant& value);

    /**
     * Add a property. This allows to add more than one value for a property.
     * \param property The property to set
     * \param res The value of the property.
     */
    void addProperty(const QUrl& property, const SimpleResource& res);

    /**
     * Add a property.
     * \param property The property to set
     * \param value The value of the property. Will be converted to a QVariant.
     */
    void addPropertyNode(const QUrl& property, const Soprano::Node& value);

    void remove(const QUrl& property, const QVariant& value);
    void remove(const QUrl& property);

    /**
     * Remove all property/value pairs matchin the provided pattern. Both
     * parameters can be set to empty values to act as wildcards.
     */
    void removeAll(const QUrl& property, const QVariant& value);

    /**
     * A convenience method which adds a property of type rdf:type.
     * \param type The type to add to the resource. Must be the URI of an RDF class.
     */
    void addType(const QUrl& type);

    /**
     * A convenience method which sets the property of type rdf:type.
     * \param types The types to set to the resource. Must be URIs of RDF classes.
     */
    void setTypes(const QList<QUrl>& types);

    /**
     * Get all values for \p property.
     */
    QVariantList property(const QUrl& property) const;

    /**
     * \return All properties.
     */
    PropertyHash properties() const;

    /**
     * Converts the resource into a list of statements. None of the statements will
     * have a valid context set.
     */
    QList<Soprano::Statement> toStatementList() const;

    /**
     * \return \p true if the resource is valid, ie. if it has a valid uri() and
     * a non-empty list of properties.
     */
    bool isValid() const;


private:
    class Private;
    QSharedDataPointer<Private> d;
};


NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator<<(QDataStream &, const Nepomuk::SimpleResource& );
NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator>>(QDataStream &, Nepomuk::SimpleResource& );
NEPOMUK_DATA_MANAGEMENT_EXPORT QDebug operator<<(QDebug dbg, const Nepomuk::SimpleResource& res);
NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator<<(QDataStream &, const Nepomuk::SimpleResource& );
NEPOMUK_DATA_MANAGEMENT_EXPORT QDataStream & operator>>(QDataStream &, Nepomuk::SimpleResource& );

NEPOMUK_DATA_MANAGEMENT_EXPORT uint qHash(const SimpleResource& res);
}

#endif
