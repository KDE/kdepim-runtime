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

#include "modelstatesaver.h"

#include <KConfigGroup>
#include <KDebug>

#include <QAbstractItemModel>
#include <QByteArray>
#include <QModelIndex>
#include <QHash>
#include <QPair>
#include <QStringList>
#include <QVector>

using namespace Akonadi;

namespace {
    struct Role {
        QByteArray identifier;
        int role;
        QVariant defaultValue;
    };
}

class ModelStateSaver::Private
{
    ModelStateSaver* const q;
public:
    explicit Private( QAbstractItemModel* m, ModelStateSaver* qq ) : q( qq ), model( m ) {
        Q_ASSERT( model );
    }
    
    void rowsInserted( const QModelIndex&, int, int );

    QAbstractItemModel* const model;
    QHash<QString, QVector<QPair<int,QVariant> > > pendingProperties;
    QHash<QByteArray,Role> roles;

    void saveState( const QModelIndex& index, QHash<QByteArray,QVector<QPair<QString, QVariant> > >& values )
    {
        const QString cfgKey = q->key( index );

        Q_FOREACH( const Role& r, roles )
        {
            const QVariant v = index.data( r.role );
            kDebug() << v << r.defaultValue;
            if ( v != r.defaultValue )
                values[r.identifier].push_back( qMakePair( cfgKey, v ) );
        }
        const int rowCount = model->rowCount( index );
        for ( int i = 0; i < rowCount; ++i ) {
            const QModelIndex child = model->index( i, 0, index );
            saveState( child, values );
        }
    }

    void restoreState( const QModelIndex& index )
    {
        const QString key = q->key( index );
        if ( pendingProperties.contains( key ) ) {
            typedef QPair<int,QVariant> IntVariantPair;
            Q_FOREACH ( const IntVariantPair &i, pendingProperties.value( key ) )
                if ( index.data( i.first ) != i.second )
                    model->setData( index, i.second, i.first );
            pendingProperties.remove( key );
        }

        const int rowCount = model->rowCount( index );
        for ( int i = 0; i < rowCount && !pendingProperties.isEmpty(); ++i ) {
            const QModelIndex child = model->index( i, 0, index );
            restoreState( child );
        }
    }
};

void ModelStateSaver::Private::rowsInserted( const QModelIndex& index, int start, int end )
{
    for ( int i = start; i <= end && !pendingProperties.isEmpty(); ++i ) {
        const QModelIndex child = model->index( i, 0, index );
        restoreState( child );
    }

    if ( pendingProperties.isEmpty() )
        model->disconnect( q );
}

ModelStateSaver::ModelStateSaver( QAbstractItemModel* model, QObject* parent ) : QObject( parent ), d( new Private( model, this ) )
{
}

ModelStateSaver::~ModelStateSaver()
{
  delete d;
}

void ModelStateSaver::restoreConfig( const KConfigGroup& configGroup )
{
    Q_FOREACH ( const Role &r, d->roles ) {
        const QByteArray ck = QByteArray("Role_") + r.identifier;
        const QStringList l = configGroup.readEntry( ck.constData(), QStringList() );
        if ( l.isEmpty() )
            continue;
        if ( l.size() % 2 != 0 ) {
            kWarning() << "Ignoring invalid configuration value because of odd number of list entries:" << ck;
            continue;
        }
        QStringList::ConstIterator it = l.constBegin();
        while ( it != l.constEnd() ) {
            const QString key = *it;
            ++it;
            const QVariant value = *it;
            ++it;
            d->pendingProperties[key].append( qMakePair( r.role, value ) );
        }
    }

    // initial restore run, for everything already loaded
    for ( int i = 0; i < d->model->rowCount() && !d->pendingProperties.isEmpty(); ++i ) {
        const QModelIndex index = d->model->index( i, 0 );
        d->restoreState( index );
    }

    // watch the model for stuff coming in delayed
    if ( !d->pendingProperties.isEmpty() )
        connect( d->model, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(rowsInserted(QModelIndex,int,int)), Qt::QueuedConnection );
}

void ModelStateSaver::saveConfig( KConfigGroup& configGroup )
{
    configGroup.deleteGroup();
    typedef QPair<QString, QVariant> StringVariantPair;
    typedef QHash<QByteArray, QVector<StringVariantPair> > ValueHash;
    ValueHash values;

    const int rowCount = d->model->rowCount();
    for ( int i = 0; i < rowCount; ++i ) {
        const QModelIndex index = d->model->index( i, 0 );
        d->saveState( index, values );
    }

    ValueHash::ConstIterator it = values.constBegin();
    while ( it != values.constEnd() ) {
        QStringList l;
        Q_FOREACH( const StringVariantPair &pair, it.value() ) {
            l.push_back( pair.first );
            l.push_back( pair.second.toString() );
        }
        configGroup.writeEntry( ( QByteArray("Role_") + it.key() ).constData(), l );
        ++it;
    }
}

void ModelStateSaver::addRole( int role, const QByteArray &identifier, const QVariant &defaultValue )
{
    Role r;
    r.role = role;
    r.identifier = identifier;
    r.defaultValue = defaultValue;
    d->roles.insert( identifier, r );
}

#include "modelstatesaver.moc"
