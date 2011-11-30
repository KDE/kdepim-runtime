/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#include "simpleresource.h"

#include <QtCore/QHashIterator>
#include <QtCore/QSharedData>
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QDataStream>

#include <Soprano/Node>
#include <Soprano/LiteralValue>
#include <Soprano/Vocabulary/RDF>


namespace {
QAtomicInt s_idCnt;

QUrl createBlankUri()
{
    // convert int to string (a...z,aa...az,ba....bz,...)
    int idCnt = s_idCnt.fetchAndAddRelaxed(1);
    QByteArray id;
    do {
        const int rest = idCnt%26;
        id.append('a' + rest);
        idCnt -= rest;
        idCnt /= 26;
    } while(idCnt > 0);

    const QUrl uri = QString(QLatin1String("_:") + id);
    return uri;
}
}

class Nepomuk::SimpleResource::Private : public QSharedData
{
public:
    QUrl m_uri;
    PropertyHash m_properties;
};

Nepomuk::SimpleResource::SimpleResource(const QUrl& uri)
{
    d = new Private();
    setUri(uri);
}

Nepomuk::SimpleResource::SimpleResource(const PropertyHash& properties)
{
    d = new Private();
    setUri(QUrl());
    setProperties(properties);
}

Nepomuk::SimpleResource::SimpleResource(const SimpleResource& other)
    : d(other.d)
{
}

Nepomuk::SimpleResource::~SimpleResource()
{
}

Nepomuk::SimpleResource & Nepomuk::SimpleResource::operator=(const Nepomuk::SimpleResource &other)
{
    d = other.d;
    return *this;
}

QUrl Nepomuk::SimpleResource::uri() const
{
    return d->m_uri;
}

void Nepomuk::SimpleResource::setUri(const QUrl& uri)
{
    if(uri.isEmpty())
        d->m_uri = createBlankUri();
    else
        d->m_uri = uri;
}

namespace {
    Soprano::Node convertIfBlankNode( const Soprano::Node & n ) {
        if( n.isResource() && n.uri().toString().startsWith("_:") ) {
            return Soprano::Node( n.uri().toString().mid(2) ); // "_:" take 2 characters
        }
        return n;
    }
}

QList< Soprano::Statement > Nepomuk::SimpleResource::toStatementList() const
{
    QList<Soprano::Statement> list;
    QHashIterator<QUrl, QVariant> it( d->m_properties );
    while( it.hasNext() ) {
        it.next();

        Soprano::Node object;
        if( it.value().type() == QVariant::Url )
            object = it.value().toUrl();
        else
            object = Soprano::LiteralValue( it.value() );

        list << Soprano::Statement( convertIfBlankNode( d->m_uri ),
                                    it.key(),
                                    convertIfBlankNode( object ) );
    }
    return list;
}

bool Nepomuk::SimpleResource::isValid() const
{
    // We do not check if m_uri.isValid() as a blank uri of the form "_:daf" would be invalid
    if(d->m_uri.isEmpty() || d->m_properties.isEmpty()) {
        return false;
    }

    // properties cannot have empty values
    PropertyHash::const_iterator end = d->m_properties.constEnd();
    for(PropertyHash::const_iterator it = d->m_properties.constBegin(); it != end; ++it) {
        if(!it.value().isValid()) {
            return false;
        }
    }

    return true;
}

bool Nepomuk::SimpleResource::operator==(const Nepomuk::SimpleResource &other) const
{
    return d->m_uri == other.d->m_uri && d->m_properties == other.d->m_properties;
}

Nepomuk::PropertyHash Nepomuk::SimpleResource::properties() const
{
    return d->m_properties;
}

bool Nepomuk::SimpleResource::contains(const QUrl &property) const
{
    return d->m_properties.contains(property);
}

bool Nepomuk::SimpleResource::contains(const QUrl &property, const QVariant &value) const
{
    return d->m_properties.contains(property, value);
}

bool Nepomuk::SimpleResource::containsNode(const QUrl &property, const Soprano::Node &node) const
{
    if(node.isLiteral())
        return contains(property, node.literal().variant());
    else if(node.isResource())
        return contains(property, node.uri());
    else
        return false;
}

void Nepomuk::SimpleResource::setPropertyNode(const QUrl &property, const Soprano::Node &value)
{
    d->m_properties.remove(property);
    addPropertyNode(property, value);
}

void Nepomuk::SimpleResource::setProperty(const QUrl &property, const QVariant &value)
{
    d->m_properties.remove(property);
    addProperty(property, value);
}

void Nepomuk::SimpleResource::setProperty(const QUrl& property, const Nepomuk::SimpleResource& res)
{
    setProperty(property, res.uri());
}


void Nepomuk::SimpleResource::setProperty(const QUrl &property, const QVariantList &values)
{
    d->m_properties.remove(property);
    foreach(const QVariant& v, values) {
        addProperty(property, v);
    }
}

void Nepomuk::SimpleResource::addProperty(const QUrl &property, const QVariant &value)
{
    // QMultiHash even stores the same key/value pair multiple times!
    if(!d->m_properties.contains(property, value))
        d->m_properties.insertMulti(property, value);
}

void Nepomuk::SimpleResource::addProperty(const QUrl& property, const Nepomuk::SimpleResource& res)
{
    addProperty(property, res.uri());
}

void Nepomuk::SimpleResource::addPropertyNode(const QUrl &property, const Soprano::Node &node)
{
    if(node.isResource())
        addProperty(property, QVariant(node.uri()));
    else if(node.isLiteral())
        addProperty(property, node.literal().variant());
    // else do nothing
}

void Nepomuk::SimpleResource::remove(const QUrl &property, const QVariant &value)
{
    d->m_properties.remove(property, value);
}

void Nepomuk::SimpleResource::remove(const QUrl &property)
{
    d->m_properties.remove(property);
}

void Nepomuk::SimpleResource::removeAll(const QUrl &property, const QVariant &value)
{
    if(property.isEmpty()) {
        if(value.isValid()) {
            foreach(const QUrl& prop, d->m_properties.keys(value)) {
                d->m_properties.remove(prop, value);
            }
        }
        else {
            d->m_properties.clear();
        }
    }
    else if(value.isValid()){
        d->m_properties.remove(property, value);
    }
    else {
        d->m_properties.remove(property);
    }
}

void Nepomuk::SimpleResource::addType(const QUrl &type)
{
    addProperty(Soprano::Vocabulary::RDF::type(), type);
}

void Nepomuk::SimpleResource::setTypes(const QList<QUrl> &types)
{
    QVariantList values;
    foreach(const QUrl& type, types) {
        values << type;
    }
    setProperty(Soprano::Vocabulary::RDF::type(), values);
}

void Nepomuk::SimpleResource::setProperties(const Nepomuk::PropertyHash &properties)
{
    d->m_properties = properties;
}

void Nepomuk::SimpleResource::clear()
{
    d->m_properties.clear();
}

void Nepomuk::SimpleResource::addProperties(const Nepomuk::PropertyHash &properties)
{
    d->m_properties += properties;
}

QVariantList Nepomuk::SimpleResource::property(const QUrl &property) const
{
    return d->m_properties.values(property);
}

uint Nepomuk::qHash(const SimpleResource& res)
{
    return qHash(res.uri());
}

QDebug Nepomuk::operator<<(QDebug dbg, const Nepomuk::SimpleResource& res)
{
    return dbg << res.uri() << res.properties();
}

QDataStream & Nepomuk::operator<<(QDataStream & stream, const Nepomuk::SimpleResource& resource)
{
    stream << resource.uri() << resource.properties();
    return stream;
}

QDataStream & Nepomuk::operator>>(QDataStream & stream, Nepomuk::SimpleResource& resource)
{
    QUrl uri;
    PropertyHash properties;
    stream >> uri >> properties;
    resource.setUri(uri);
    resource.setProperties(properties);
    return stream;
}
