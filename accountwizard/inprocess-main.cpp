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

#include "dialog.h"
#include "global.h"

#include <kglobal.h>
#include <QtCore/QObject>
#include <QtCore/QtPlugin>
#include <QtCore/QStringList>

class AccountWizard : public QObject
{
  Q_OBJECT
  public:
    explicit AccountWizard( QObject* parent = 0 ) : QObject( parent ) {}

  public slots:
    void run( const QStringList &types, QWidget *parent )
    {
      if ( !types.isEmpty() )
        Global::setTypeFilter( types );
      Dialog dlg( parent );
      dlg.exec();
    }
};

Q_EXPORT_PLUGIN2( accountwizard, AccountWizard )

#include "inprocess-main.moc"
