/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

    ~EntityCompactChangeAttribute() override;

    void setRemoteId(const QString &remoteId);

    QString remoteId() const;

    void setRemoteRevision(const QString &remoteRev);

    QString remoteRevision() const;

public:
    QByteArray type() const override;

    EntityCompactChangeAttribute *clone() const override;

    QByteArray serialized() const override;

    void deserialize(const QByteArray &data) override;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};
}
}

#endif
