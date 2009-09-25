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

#ifndef SCRIPT_H
#define SCRIPT_H

#include <kross/core/action.h>
#include <QHash>

class Script : public QObject
{
  Q_OBJECT
  public:
    Script();
    void configure(const QString &path);
    void insertObject(QObject *object, const QString &objectName);

  public slots:
    Q_SCRIPTABLE void include( const QString &path );

  private slots:
    void start();
    void finished( Kross::Action *action );

  private:
    Kross::Action *action;
};

#endif
