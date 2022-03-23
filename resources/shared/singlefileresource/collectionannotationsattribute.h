/*
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadi-singlefileresource_export.h"
#include <Akonadi/Attribute>
#include <QMap>

namespace Akonadi
{
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
