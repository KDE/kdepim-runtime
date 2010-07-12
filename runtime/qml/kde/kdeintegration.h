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

#ifndef KDEINTEGRATION_H
#define KDEINTEGRATION_H

#include <QObject>
#include <QVariant>
#include <QStringList>

class QIcon;
class QPixmap;
class QScriptValue;
class QScriptContext;

class KDEIntegration : public QObject
{
  Q_OBJECT

  public:
    explicit KDEIntegration(QObject* parent = 0);

  public slots:
    QString i18n( const QScriptValue &array );
    QString i18nc( const QScriptValue &array );
    QString i18np( const QScriptValue &array );
    QString i18ncp( const QScriptValue &array );

    QString iconPath( const QString &iconName, int size );
    QPixmap iconToPixmap( const QIcon &icon, int size );

    QString locate( const QString &type, const QString &filename );

  private:
    QScriptContext *getContext( const QScriptValue &v );
};

#endif

