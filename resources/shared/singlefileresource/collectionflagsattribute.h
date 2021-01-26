/*
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONFLAGSATTRIBUTE_H
#define AKONADI_COLLECTIONFLAGSATTRIBUTE_H

#include "akonadi-singlefileresource_export.h"
#include <attribute.h>

namespace Akonadi
{
class AKONADI_SINGLEFILERESOURCE_EXPORT CollectionFlagsAttribute : public Akonadi::Attribute
{
public:
    explicit CollectionFlagsAttribute(const QList<QByteArray> &flags = QList<QByteArray>());
    void setFlags(const QList<QByteArray> &flags);
    QList<QByteArray> flags() const;
    QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QList<QByteArray> mFlags;
};
}

#endif
