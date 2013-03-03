/*
    KRssLocalResource - an Akonadi resource to store subscriptions in an opml file
    Copyright (C) 2013    Frank Osterfeld <osterfeld@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RSSCOLLECTIONATTRIBUTE_H
#define RSSCOLLECTIONATTRIBUTE_H

#include <Akonadi/Attribute>

class RssCollectionAttribute : public Akonadi::Attribute
{
public:
    RssCollectionAttribute();
    QByteArray type() const;
    RssCollectionAttribute* clone() const;

    void setFetchCounter( int counter );
    int fetchCounter() const;

    QByteArray serialized() const;
    void deserialize( const QByteArray &data );

private:
    int m_fetchCounter;
};

#endif

