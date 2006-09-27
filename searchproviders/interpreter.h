/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * This class encapsulates solving nexted boolean constructs
 */
template <typename T>
class InterpreterItem
{
  public:
    enum Relation
    {
      And,
      Or
    };

    /**
     * Creates a new solver item with the given key, comparator and pattern.
     */
    InterpreterItem( const QString &key, const QString &comparator, const QString &pattern )
      : mKey( key ), mComparator( comparator ), mPattern( pattern ), mIsLeaf( true )
    {
    }

    /**
     * Creates a new solver item which combines the given child items by a given relation.
     *
     * Ownership of the child items is transfered to the item.
     */
    InterpreterItem( Relation relation )
      : mRelation( relation )
    {
    }

    virtual void setChildItems( const QList<InterpreterItem<T>*> items )
    {
      mItems = items;
    }

    /**
     * Destroys the solver item and all its child items.
     */
    virtual ~InterpreterItem()
    {
      qDeleteAll( mItems );
      mItems.clear();
    }

    /**
     * Starts the resolution of the boolean construct for a given object.
     */
    bool solve( const T* object ) const
    {
      if ( mIsLeaf )
        return matches( mKey, mComparator, mPattern, object );

      if ( mRelation == And ) {
        for ( int i = 0; i < mItems.count(); ++i ) {
          if ( !mItems[ i ]->solve( object ) )
            return false;
        }

        return true;
      } else { // Or
        for ( int i = 0; i < mItems.count(); ++i ) {
          if ( mItems[ i ]->solve( object ) )
            return true;
        }

        return false;
      }
    }

    QString dump() const
    {
      if ( mIsLeaf ) {
        return QString( "(%1 %2 %3)" ).arg( mKey, mComparator, mPattern );
      } else {
        QString text = "(";
        text += ( mRelation == And ? "&" : "|" );
        for ( int i = 0; i < mItems.count(); ++i )
          text += mItems[ i ]->dump();
        text += ")";

        return text;
      }
    }

  protected:
    /**
     * This method must be reimplemented to do the actual comparision for the specific type.
     */
    virtual bool matches( const QString &key, const QString &comparator, const QString &pattern, const T* object ) const = 0;

  private:
    QString mKey;
    QString mComparator;
    QString mPattern;

    Relation mRelation;
    QList<InterpreterItem<T>*> mItems;

    bool mIsLeaf;
};

class Parser
{
  public:
    template <typename T>
    T* parse( const QString &query ) const
    {
      QString pattern( query.trimmed() );

      if ( pattern.isEmpty() ) {
        qDebug( "empty search pattern passed!" );
        return 0;
      }

      const QStringList tokens = tokenize( pattern );

      QStringListIterator it( tokens );

      return parseInternal<T>( it );
    };

  private:
    QStringList tokenize( const QString& ) const;
    QStringListIterator balanced( QStringListIterator ) const;

    template <typename T>
    T* parseInternal( QStringListIterator &it ) const
    {
      if ( !it.hasNext() )
        return 0;

      QString token = it.next();
      if ( token == QLatin1String( "&" ) || token == QLatin1String( "|" ) ) {
        /**
         * We have a term like: (&( ... )( ... )( ... ))
         */
        QList<T*> childItems;

        const QString operatorSign = token;

        do {
          QStringListIterator nextIt = balanced( it );
          if ( !nextIt.hasNext() ) {
            qDeleteAll( childItems );
            return 0;
          }
          token = nextIt.next();

          if ( !nextIt.hasPrevious() ) {
            qDeleteAll( childItems );
            return 0;
          }
          nextIt.previous();

          T *item = parseInternal<T>( it );
          if ( item == 0 ) {
            qDeleteAll( childItems );
            return 0;
          }

          childItems.append( item );

          if ( token[ 0 ] != '(' )
            break;

          it = nextIt;
        } while ( true );

        T *item = 0;

        if ( operatorSign == QLatin1String( "&" ) )
          item = new T( T::And );
        else
          item = new T( T::Or );

        item->setChildItems( childItems );

        return item;
      } else if ( token[ 0 ] == '(' ) {
        /**
         * We have a term like: ( ... )
         */
        return parseInternal<T>( it );
      } else {
        /**
         * We have a term like: a = b
         */
        const QString key = token;

        if ( !it.hasNext() )
          return 0;

        const QString comparator = it.next();

        if ( !it.hasNext() )
          return 0;

        const QString pattern = it.next();

        return new T( key, comparator, pattern );
      }

      return 0;
    }
};

#endif
