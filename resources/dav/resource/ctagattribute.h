/*
    Copyright (c) 2015 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef CTAGATTRIBUTE_H
#define CTAGATTRIBUTE_H

#include <AkonadiCore/attribute.h>

#include <QtCore/QString>

class CTagAttribute : public Akonadi::Attribute
{
public:
    explicit CTagAttribute(const QString &ctag = QString());

    void setCTag(const QString &ctag);
    QString CTag() const;

    Akonadi::Attribute *clone() const Q_DECL_OVERRIDE;
    QByteArray type() const Q_DECL_OVERRIDE;
    QByteArray serialized() const Q_DECL_OVERRIDE;
    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;

private:
    QString mCTag;
};

#endif