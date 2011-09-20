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


#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H
#include <KDE/KMimeType>
#include <QtCore/QVector>
#include <kmimetypetrader.h>
#include <kservice.h>

#include <nepomukfeederplugin.h>

//Cache/Find plugins
class FeederPluginloader
{
private:
  FeederPluginloader(){};
  FeederPluginloader(FeederPluginloader const&); // Don't Implement
  void operator=(FeederPluginloader const&); // Don't implement
public:
  static FeederPluginloader &instance()
  {
      static FeederPluginloader inst;
      return inst;
  }

  QList < QSharedPointer<Akonadi::NepomukFeederPlugin> > feederPluginsForMimeType( const QString &mimetype );

private:
  QHash<QString, QSharedPointer<Akonadi::NepomukFeederPlugin> > m_plugins;
  QStringList m_noPluginList;
};

#endif // PLUGINLOADER_H
