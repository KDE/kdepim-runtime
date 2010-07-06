/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "kdeintegrationplugin.h"
#include "kdeintegration.h"

#include <kdebug.h>
#include <QtDeclarative/qdeclarative.h>
#include <qdeclarativecontext.h>
#include <qdeclarativeengine.h>

KDEIntegrationPlugin::KDEIntegrationPlugin(QObject* parent) : QDeclarativeExtensionPlugin( parent )
{
  qDebug() << Q_FUNC_INFO;
  kDebug(); 
}

void KDEIntegrationPlugin::registerTypes(const char* uri)
{
  kDebug() << uri;
//  qmlRegisterType<KDEIntegration>( uri, 4, 5, "KDE" );
}

void KDEIntegrationPlugin::initializeEngine(QDeclarativeEngine *engine, const char* uri)
{
  kDebug() << engine << uri;
  engine->rootContext()->setContextProperty( QLatin1String("KDE"), new KDEIntegration( engine ) );
}

#include "kdeintegrationplugin.moc"

Q_EXPORT_PLUGIN2( kdeintegrationplugin, KDEIntegrationPlugin )
