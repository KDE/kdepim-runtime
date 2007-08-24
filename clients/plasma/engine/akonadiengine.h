/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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


#ifndef AKONADIENGINE_H
#define AKONADIENGINE_H

#include <plasma/dataengine.h>

#include <libakonadi/item.h>

class KJob;

class AkonadiEngine : public Plasma::DataEngine
{
  Q_OBJECT

  public:
    AkonadiEngine( QObject* parent, const QStringList& args );
    ~AkonadiEngine();

  private slots:
    void itemAdded( const Akonadi::Item &item );
    void itemChanged( const Akonadi::Item &item );
    void fetchDone( KJob* job );
};

K_EXPORT_PLASMA_DATAENGINE(akonadi, AkonadiEngine)

#endif
