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

#include "dbinitializer.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlField>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

DbInitializer::DbInitializer( const QSqlDatabase &database, const QString &templateFile )
  : mDatabase( database ), mTemplateFile( templateFile )
{
}

DbInitializer::~DbInitializer()
{
}

bool DbInitializer::run()
{
  qDebug() << "DbInitializer::run()";

  QFile file( mTemplateFile );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to open template file '%1'." ).arg( mTemplateFile );
    return false;
  }

  QDomDocument document;

  QString errorMsg;
  int line, column;
  if ( !document.setContent( &file, &errorMsg, &line, &column ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to parse template file '%1': %2 (%3:%4)." )
                       .arg( mTemplateFile ).arg( errorMsg ).arg( line ).arg( column );
    return false;
  }

  const QDomElement documentElement = document.documentElement();
  if ( documentElement.tagName() != QLatin1String( "database" ) ) {
    mErrorMsg = QString::fromLatin1( "Invalid format of template file '%1'." ).arg( mTemplateFile );
    return false;
  }

  QDomElement tableElement = documentElement.firstChildElement();
  while ( !tableElement.isNull() ) {
    if ( tableElement.tagName() == QLatin1String( "table" ) ) {
      if ( !checkTable( tableElement ) )
        return false;
    } else if ( tableElement.tagName() == QLatin1String( "relation" ) ) {
      if ( !checkRelation( tableElement ) )
        return false;
    }else {
      mErrorMsg = QString::fromLatin1( "Unknown tag, expected <table> and got <%1>." ).arg( tableElement.tagName() );
      return false;
    }

    tableElement = tableElement.nextSiblingElement();
  }

  qDebug() << "DbInitializer::run() done";
  return true;
}

bool DbInitializer::checkTable( const QDomElement &element )
{
  const QString tableName = element.attribute( QLatin1String("name") ) + QLatin1String("Table");

  qDebug() << "checking table " << tableName;

  typedef QPair<QString, QString> ColumnEntry;

  QList<ColumnEntry> columnsList;
  QStringList dataList;

  QDomElement columnElement = element.firstChildElement();
  while ( !columnElement.isNull() ) {
    if ( columnElement.tagName() == QLatin1String( "column" ) ) {
      ColumnEntry entry;
      entry.first = columnElement.attribute( QLatin1String("name") );
      if ( columnElement.attribute( QLatin1String("sqltype") ).isEmpty() )
        entry.second = sqlType( columnElement.attribute( QLatin1String("type") ) ) + QLatin1String( " " ) + columnElement.attribute( QLatin1String("properties") );
      else
        entry.second = columnElement.attribute( QLatin1String("sqltype") ) + QLatin1String( " " ) + columnElement.attribute( QLatin1String("properties") );
      if ( mDatabase.driverName() == QLatin1String( "QPSQL" ) ) {
        if ( entry.second.contains( QLatin1String("AUTOINCREMENT") ) )
          entry.second = QLatin1String("SERIAL PRIMARY KEY NOT NULL");
        if ( entry.second.contains( QLatin1String("BLOB") ) )
          entry.second = QLatin1String("BYTEA");
        if ( entry.second.startsWith( QLatin1String("CHAR") ) )
          entry.second.replace(QLatin1String("CHAR"), QLatin1String("VARCHAR"));
      } else if ( mDatabase.driverName().startsWith( QLatin1String("QMYSQL") ) ) {
        if ( entry.second.contains( QLatin1String("AUTOINCREMENT") ) )
          entry.second.replace(QLatin1String("AUTOINCREMENT"), QLatin1String("AUTO_INCREMENT"));
        if ( entry.second.startsWith( QLatin1String("CHAR") ) )
          entry.second.replace(QLatin1String("CHAR"), QLatin1String("VARCHAR"));
      }
      columnsList.append( entry );
    } else if ( columnElement.tagName() == QLatin1String( "data" ) ) {
      QString values = columnElement.attribute( QLatin1String("values") );
      if ( mDatabase.driverName().startsWith( QLatin1String("QMYSQL") ) )
        values.replace( QLatin1String("\\"), QLatin1String("\\\\") );
      QString statement = QString::fromLatin1( "INSERT INTO %1 (%2) VALUES (%3)" )
          .arg( tableName )
          .arg( columnElement.attribute( QLatin1String("columns") ) )
          .arg( values );
      dataList << statement;
    }

    columnElement = columnElement.nextSiblingElement();
  }

  QSqlQuery query( mDatabase );

  if ( !hasTable( tableName ) ) {
    /**
     * We have to create the entire table.
     */

    QString columns;
    for ( int i = 0; i < columnsList.count(); ++i ) {
      if ( i != 0 )
        columns.append( QLatin1String(", ") );

      columns.append( columnsList[ i ].first + QLatin1Char(' ') + columnsList[ i ].second );
    }

    /**
     * Add optional extra table properties (such as foreign keys and cascaded updates/deletes)
     */
    if( element.hasAttribute( QLatin1String("properties") ) )
      columns.append( QLatin1String(", ") + element.attribute( QLatin1String("properties") ) );

    const QString statement = QString::fromLatin1( "CREATE TABLE %1 (%2);" ).arg( tableName, columns );
    qDebug() << statement;

    if ( !query.exec( statement ) ) {
      mErrorMsg = QLatin1String( "Unable to create entire table." );
      return false;
    }
  } else {
    /**
     * Check for every column whether it exists.
     */

    const QSqlRecord table = mDatabase.record( tableName );

    for ( int i = 0; i < columnsList.count(); ++i ) {
      const ColumnEntry entry = columnsList[ i ];

      bool found = false;
      for ( int j = 0; j < table.count(); ++j ) {
        const QSqlField column = table.field( j );

        if ( columnsList[ i ].first.toLower() == column.name().toLower() ) {
          found = true;
        }
      }

      if ( !found ) {
        /**
         * Add missing column to table.
         */
        const QString statement = QString::fromLatin1( "ALTER TABLE %1 ADD COLUMN %2 %3;" )
                                         .arg( tableName, entry.first, entry.second );

        if ( !query.exec( statement ) ) {
          mErrorMsg = QString::fromLatin1( "Unable to add column '%1' to table '%2'." ).arg( entry.first, tableName );
          return false;
        }
      }
    }

    // TODO: remove obsolete columns (when sqlite will support it) and adapt column type modifications
  }

  // add indices
  columnElement = element.firstChildElement();
  while ( !columnElement.isNull() ) {
    if ( columnElement.tagName() == QLatin1String( "index" ) ) {
      if ( !hasIndex( tableName, columnElement.attribute( QLatin1String("name") ) ) ) {
        QString statement = QLatin1String( "CREATE " );
        if ( columnElement.attribute( QLatin1String("unique") ) == QLatin1String( "true" ) )
          statement += QLatin1String( "UNIQUE " );
        statement += QLatin1String( "INDEX " );
        statement += columnElement.attribute( QLatin1String("name") );
        statement += QLatin1String( " ON " );
        statement += tableName;
        statement += QLatin1String( " (" );
        statement += columnElement.attribute( QLatin1String("columns") );
        statement += QLatin1Char(')');
        QSqlQuery query( mDatabase );
        qDebug() << "adding index" << statement;
        if ( !query.exec( statement ) ) {
          mErrorMsg = QLatin1String( "Unable to create index." );
          return false;
        }
      }
    }
    columnElement = columnElement.nextSiblingElement();
  }


  // add initial data if table is empty
  const QString statement = QString::fromLatin1( "SELECT * FROM %1 LIMIT 1" ).arg( tableName );
  if ( !query.exec( statement ) ) {
    mErrorMsg = QString::fromLatin1( "Unable to retrieve data from table '%1'." ).arg( tableName );
    return false;
  }
  if ( query.size() == 0  || !query.first() ) {
    foreach ( QString stmt, dataList ) {
      if ( !query.exec( stmt ) ) {
        mErrorMsg = QString::fromLatin1( "Unable to add initial data to table '%1'." ).arg( tableName );
        mErrorMsg += QString::fromLatin1( "Query was: %1" ).arg( stmt );
        return false;
      }
    }
  }

  return true;
}

bool DbInitializer::checkRelation(const QDomElement & element)
{
  QString table1 = element.attribute(QLatin1String("table1"));
  QString table2 = element.attribute(QLatin1String("table2"));
  QString col1 = element.attribute(QLatin1String("column1"));
  QString col2 = element.attribute(QLatin1String("column2"));

  QString tableName = table1 + table2 + QLatin1String("Relation");
  qDebug() << "checking relation " << tableName;

  if ( !hasTable( tableName ) ) {
    QString statement = QString::fromLatin1( "CREATE TABLE %1 (" ).arg( tableName );
    statement += QString::fromLatin1("%1_%2 INTEGER REFERENCES %3(%4), " )
        .arg( table1 )
        .arg( col1 )
        .arg( table1 )
        .arg( col1 );
    statement += QString::fromLatin1("%1_%2 INTEGER REFERENCES %3(%4))" )
        .arg( table2 )
        .arg( col2 )
        .arg( table2 )
        .arg( col2 );
    qDebug() << statement;

    QSqlQuery query( mDatabase );
    if ( !query.exec( statement ) ) {
      mErrorMsg = QLatin1String( "Unable to create entire table." );
      return false;
    }
  }

  if ( !hasIndex( tableName, QLatin1String("PRIMARY") ) ) {
    QString statement = QLatin1String( "ALTER TABLE " );
    statement += tableName;
    statement += QLatin1String( " ADD PRIMARY KEY (" );
    statement += QString::fromLatin1( "%1_%2" ).arg( table1 ).arg( col1 );
    statement += QLatin1String( ", " );
    statement += QString::fromLatin1( "%1_%2" ).arg( table2 ).arg( col2 );
    statement += QLatin1Char(')');
    QSqlQuery query( mDatabase );
    qDebug() << "adding index" << statement;
    if ( !query.exec( statement ) ) {
      mErrorMsg = QLatin1String( "Unable to create index." );
      return false;
    }
  }

  return true;
}

QString DbInitializer::errorMsg() const
{
  return mErrorMsg;
}

QString DbInitializer::sqlType(const QString & type)
{
  if ( type == QLatin1String("int") )
    return QLatin1String("INTEGER");
  if ( type == QLatin1String("qint64") )
    return QLatin1String("BIGINT");
  if ( type == QLatin1String("QString") )
    return QLatin1String("TEXT");
  if (type == QLatin1String("QByteArray") )
    return QLatin1String("LONGBLOB");
  if ( type == QLatin1String("QDateTime") )
    return QLatin1String("TIMESTAMP");
  if ( type == QLatin1String( "bool" ) )
    return QLatin1String("BOOL");
  Q_ASSERT( false );
  return QString();
}

bool DbInitializer::hasTable(const QString & tableName)
{
  QString statement;
  if ( mDatabase.driverName() == QLatin1String( "QPSQL" ) )
    statement = QLatin1String("SELECT tablename FROM pg_tables ORDER BY tablename;" );
  else if ( mDatabase.driverName() == QLatin1String( "QSQLITE" ) )
    statement = QLatin1String("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
  else if ( mDatabase.driverName().startsWith( QLatin1String("QMYSQL") ) )
    statement = QLatin1String("SHOW TABLES");
  else
    Q_ASSERT( false ); // unknown driver

  QSqlQuery query( mDatabase );
  if ( !query.exec( statement ) ) {
    mErrorMsg = QLatin1String( "Unable to retrieve table information from database." );
    return false;
  }

  while ( query.next() ) {
    if ( query.value( 0 ).toString().toLower() == tableName.toLower() )
      return true;
  }
  return false;
}

bool DbInitializer::hasIndex(const QString & tableName, const QString & indexName)
{
  if ( mDatabase.driverName().startsWith( QLatin1String("QMYSQL") ) ) {
    QString statement = QString::fromLatin1("SHOW INDEXES FROM %1").arg( tableName );
    QSqlQuery query( mDatabase );
    if ( !query.exec( statement ) ) {
      mErrorMsg = QLatin1String( "Unable to list index information." );
      return false;
    }
    while ( query.next() ) {
      // FIXME: use column name (key_name) instead of column index!!
      if ( query.value(2) == indexName )
        return true;
    }
    return false;
  } else {
    qFatal("Implement index support for your database!");
  }
  return false;
}
