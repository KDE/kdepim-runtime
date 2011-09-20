/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "pluginloader.h"
#include "plugins/nepomuknotefeeder.h"


QList< QSharedPointer< Akonadi::NepomukFeederPlugin > > FeederPluginloader::feederPluginsForMimeType(const QString& mimetype)
{
  //kDebug() << mimetype;
  if (m_plugins.contains(mimetype)) {
    //kDebug() << "cached plugin found";
    return m_plugins.values(mimetype);
  } else if ( m_noPluginList.contains(mimetype) ) {
    kDebug() << "No feeder for type " << mimetype << " found";
    return QList< QSharedPointer< Akonadi::NepomukFeederPlugin > >();
  }

  KService::List lst = KMimeTypeTrader::self()->query(mimetype, QString::fromLatin1("AkonadiNepomukFeeder"));
  if (lst.isEmpty()) {
    kDebug() << "No feeder for type " << mimetype << " found";
    m_noPluginList.append(mimetype);
    return QList< QSharedPointer< Akonadi::NepomukFeederPlugin > >();
  }
  QList< QSharedPointer< Akonadi::NepomukFeederPlugin > > pluginList;
  foreach (KService::Ptr ptr, lst) {;
    QString error;
    QSharedPointer<Akonadi::NepomukFeederPlugin> plugin(ptr->createInstance<Akonadi::NepomukFeederPlugin>(0, QVariantList(), &error));
    if (plugin.isNull()) {
      kDebug() << "could not create " << error;
      continue;
    }
    m_plugins.insertMulti(mimetype, plugin);
    pluginList << plugin;
    kDebug() << "created new plugin";
  }
  return pluginList;
}


