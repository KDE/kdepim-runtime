/*
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONANNOTATIONSATTRIBUTE_H
#define AKONADI_COLLECTIONANNOTATIONSATTRIBUTE_H

#include <attribute.h>
#include "akonadi-singlefileresource_export.h"
#include <QMap>

namespace Akonadi {
class AKONADI_SINGLEFILERESOURCE_EXPORT CollectionAnnotationsAttribute : public Akonadi::Attribute
{
public:
    CollectionAnnotationsAttribute();
    CollectionAnnotationsAttribute(const QMap<QByteArray, QByteArray> &annotations);
    void setAnnotations(const QMap<QByteArray, QByteArray> &annotations);
    QMap<QByteArray, QByteArray> annotations() const;
    QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QMap<QByteArray, QByteArray> mAnnotations;
};
}

#endif
