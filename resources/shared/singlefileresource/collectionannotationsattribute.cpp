/*
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionannotationsattribute.h"

#include <QByteArray>
#include <attribute.h>

using namespace Akonadi;

CollectionAnnotationsAttribute::CollectionAnnotationsAttribute()
{
}

CollectionAnnotationsAttribute::CollectionAnnotationsAttribute(const QMap<QByteArray, QByteArray> &annotations)
    : mAnnotations(annotations)
{
}

void CollectionAnnotationsAttribute::setAnnotations(const QMap<QByteArray, QByteArray> &annotations)
{
    mAnnotations = annotations;
}

QMap<QByteArray, QByteArray> CollectionAnnotationsAttribute::annotations() const
{
    return mAnnotations;
}

QByteArray CollectionAnnotationsAttribute::type() const
{
    static const QByteArray sType("collectionannotations");
    return sType;
}

Akonadi::Attribute *CollectionAnnotationsAttribute::clone() const
{
    return new CollectionAnnotationsAttribute(mAnnotations);
}

QByteArray CollectionAnnotationsAttribute::serialized() const
{
    QByteArray result = "";

    QMap<QByteArray, QByteArray>::const_iterator it = mAnnotations.constBegin();
    const QMap<QByteArray, QByteArray>::const_iterator end = mAnnotations.constEnd();
    for (; it != end; ++it) {
        result += it.key();
        result += ' ';
        result += it.value();
        result += " % "; // We use this separator as '%' is not allowed in keys or values
    }
    result.chop(3);

    return result;
}

void CollectionAnnotationsAttribute::deserialize(const QByteArray &data)
{
    mAnnotations.clear();
    const QList<QByteArray> lines = data.split('%');

    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines[i];
        if (i != 0 && line.startsWith(' ')) {
            line.remove(0, 1);
        }
        if (i != lines.size() - 1 && line.endsWith(' ')) {
            line.chop(1);
        }
        if (line.trimmed().isEmpty()) {
            continue;
        }
        int wsIndex = line.indexOf(' ');
        if (wsIndex > 0) {
            const QByteArray key = line.mid(0, wsIndex);
            const QByteArray value = line.mid(wsIndex + 1);
            mAnnotations[key] = value;
        } else {
            mAnnotations.insert(line, QByteArray());
        }
    }
}
