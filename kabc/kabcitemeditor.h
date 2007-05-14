/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef KABCITEMEDITOR_H
#define KABCITEMEDITOR_H

#include <QtGui/QWidget>

#include <kdepim_export.h>

namespace Akonadi {
class DataReference;
class Item;
}

class AKONADI_EXPORT KABCItemEditor : public QWidget
{
  Q_OBJECT

  public:
    KABCItemEditor( QWidget *parent = 0 );
    virtual ~KABCItemEditor();

    void setUid( const Akonadi::DataReference &uid );

  public Q_SLOTS:
    void save();

  private:
    class Private;
    Private* const d;

    Q_DISABLE_COPY( KABCItemEditor )

    Q_PRIVATE_SLOT( d, void fetchDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void storeDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemChanged( const Akonadi::Item&, const QStringList& ) )
};

#endif
