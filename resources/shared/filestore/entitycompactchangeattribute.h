/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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
*/

#ifndef AKONADI_FILESTORE_ENTITYCOMPACTCHANGEATTRIBUTE_H
#define AKONADI_FILESTORE_ENTITYCOMPACTCHANGEATTRIBUTE_H

#include "akonadi-filestore_export.h"

#include <Attribute>

namespace Akonadi
{

namespace FileStore
{

class AKONADI_FILESTORE_EXPORT EntityCompactChangeAttribute : public Attribute
{
public:
    EntityCompactChangeAttribute();

    ~EntityCompactChangeAttribute();

    void setRemoteId(const QString &remoteId);

    QString remoteId() const;

    void setRemoteRevision(const QString &remoteRev);

    QString remoteRevision() const;

public:
    QByteArray type() const Q_DECL_OVERRIDE;

    EntityCompactChangeAttribute *clone() const Q_DECL_OVERRIDE;

    QByteArray serialized() const Q_DECL_OVERRIDE;

    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

}

#endif

