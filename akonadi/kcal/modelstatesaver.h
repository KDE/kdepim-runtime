/*
    Copyright (c) 2009 KDAB
    Author: Frank Osterfeld <frank@kdab.net>

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

#ifndef AKONADI_MODELSTATESAVER_H
#define AKONADI_MODELSTATESAVER_H

#include <akonadi-kcal_export.h>

#include <QtCore/QObject>
#include <QtCore/QVariant>

class QAbstractItemModel;
class QByteArray;
class QModelIndex;
class QString;

class KConfigGroup;

namespace Akonadi {
    class AKONADI_KCAL_EXPORT ModelStateSaver : public QObject {
        Q_OBJECT
    public:
        explicit ModelStateSaver( QAbstractItemModel* model, QObject* parent=0 );
        ~ModelStateSaver();

        void saveConfig( KConfigGroup& config );

        void restoreConfig( const KConfigGroup& config );

        /**
        * adds a role to be saved and restored.
        *
        * @param role the role to save/restore
        * @param identifier Identifier used internally to write/read the role values. Must only contain letters (a-z) or digits (0-9)!
        * @param defaultValue The value that should be set if nothing is stored in the settings. When saving,
        *        values equal to the default value are not explicitely stored
        */
        void addRole( int role, const QByteArray& identifier, const QVariant& defaultValue=QVariant() );

    protected:
        virtual QString key( const QModelIndex& index ) const = 0;

    private:
        class Private;
        Private* const d;
        Q_PRIVATE_SLOT( d, void rowsInserted( const QModelIndex&, int, int ) )
  };
}

#endif // AKONADI_MODELSTATESAVER_H
