/*
  This file is part of the Grantlee template system.

  Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 3 only, as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License version 3 for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef AKONADI_TEMPLATE_LOADER_H
#define AKONADI_TEMPLATE_LOADER_H

#include <grantlee/engine.h>
#include <grantlee/grantlee_export.h>

#include <akonadi/item.h>

namespace Akonadi
{
class EntityTreeModel;
class Monitor;
}

class GRANTLEE_EXPORT AkonadiTemplateLoader : public AbstractTemplateLoader
{
  Q_OBJECT
public:
  AkonadiTemplateLoader(Akonadi::Monitor *monitor, QObject *parent = 0 );

  Template *loadByName( const QString &name ) const;
  MutableTemplate *loadMutableByName( const QString &name ) const;

private:
  Akonadi::Item getItem( const QString &name ) const;

  Akonadi::EntityTreeModel *m_model;
  QString m_themeName;
};

#endif

