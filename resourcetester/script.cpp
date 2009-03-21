/*
 * Copyright (c) 2009  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "script.h"
#include <qcoreapplication.h>

Script::Script()
{
  action = new Kross::Action(this, "ResourceTester");
  connect( action, SIGNAL(finished(Kross::Action*)), QCoreApplication::instance(), SLOT(quit()) );
}

void Script::configure(const QString &path, QHash<QString, QObject * > hash)
{
  action->setFile(path);
 
  QHashIterator<QString, QObject * > i(hash);

  while( i.hasNext()) {
    i.next();
    action->addObject( i.value(), i.key());
  }
}

void Script::configure(const QString &path)
{
  action->setFile(path);
}

void Script::insertObject(QObject *object, const QString &objectName)
{
  action->addObject(object, objectName);
}

void Script::start()
{
  action->trigger();
}

#include "script.moc"
