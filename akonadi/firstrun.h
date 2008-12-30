/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_FIRSTRUN_H
#define AKONADI_FIRSTRUN_H

#include <QObject>
#include <QStringList>
#include <QVariant>

class KConfig;
class KJob;
class QMetaObject;

namespace Akonadi {

/**
 Takes care of setting up default resource agents when running Akonadi for the first time.
*/
class Firstrun : public QObject
{
  Q_OBJECT
  public:
    Firstrun( QObject * parent = 0 );
    ~Firstrun();

  private:
    void findPendingDefaults();
    void setupNext();
    static QVariant::Type argumentType( const QMetaObject *mo, const QString &method );

  private slots:
    void instanceCreated( KJob* job );

  private:
    QStringList mPendingDefaults;
    KConfig *mConfig;
    KConfig *mCurrentDefault;
};

}

#endif

