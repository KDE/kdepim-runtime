/*
    Copyright 2011 Sebastian Kügler <sebas@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "ispdbengine.h"

#define RESULT_LIMIT 10

class IspdbEnginePrivate
{
public:
    QString query;
    QSize previewSize;
    QHash<QString, QString> icons;
};


IspdbEngine::IspdbEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent)
{
    Q_UNUSED(args);
    d = new IspdbEnginePrivate;
    setMaxSourceCount(RESULT_LIMIT); // Guard against loading too many connections
    init();
}

QString IspdbEngine::icon(const QStringList &types)
{
    // keep searching until the most specific icon is found
    return QString();
}

void IspdbEngine::init()
{
}

IspdbEngine::~IspdbEngine()
{
    delete d;
}

QStringList IspdbEngine::sources() const
{
    return QStringList();
}

bool IspdbEngine::sourceRequestEvent(const QString &name)
{
    return true;
}
#include "ispdbengine.moc"
