/* This file is part of the KDE project

   Copyright (C) 2006-2007 KovoKs <info@kovoks.nl>

   This file is based on digikams albumdb which is
   Copyright 2004 by Renchi Raju <no_working@address.known>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// Own
#include "db.h"

// C ANSI
extern "C"
{
#include <sys/time.h>
#include <time.h>
}

// C++
#include <cstdio>
#include <cstdlib>

// Qt
#include <QDir>
#include <QMap>
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QSqlDriver>

// KDE
#include <kdebug.h>
#include <KStandardDirs>

DB::DB()
{
    m_db = QSqlDatabase::addDatabase( "QSQLITE","mailodycerts" );
    setDBPath( KStandardDirs::locateLocal( "appdata","mailody4-certs.db" ) );
}

DB::~DB()
{
    if ( m_db.isOpen() )
        m_db.close();
}

void DB::setDBPath( const QString& path )
{
    if ( m_db.isOpen() )
        m_db.close();


    m_db.setDatabaseName( path );
    bool open = m_db.open();
    if ( !open )
        kFatal(  ) << "Cannot open database " << path
        << m_db.lastError().text();
    else {
        kDebug(  ) << "Database is opened: " << path;
        initDB();
    }
}

void DB::initDB()
{
    // Check if we have the required tables

    QSqlQuery res = execSql( "SELECT name FROM sqlite_master"
                             " WHERE type='table'"
                             " ORDER BY name;" );
    QStringList values;
    while ( res.next() )
        values.append( res.value( 0 ).toString() );

    if ( !values.contains( "certificates" ) )
        execSql( QString( "CREATE TABLE certificates"
                          " (cert BLOB, error BLOB, UNIQUE( cert, error ) );" ) );
}

QSqlQuery DB::execSql( const QString& sql )
{
    kDebug(  ) << "SQL-query: " << sql << endl;

    if ( !m_db.isOpen() ) {
        kFatal() << "Database is not open";
        return false;
    }

    QSqlQuery query( sql, m_db );
    if ( query.lastError().isValid() )
        kWarning(  ) << "Error in the query:" << query.lastError().text() << sql;

    return( query );
}

QString DB::escapeString( const QString& str ) const
{
    QString sting = str;
    sting.replace( "'", "''" );
    return sting;
}

bool DB::hasCert( const QString& cert, const QString& error )
{
    QSqlQuery res = execSql( QString( "SELECT cert FROM certificates where cert='%1'"
                                      " and error='%2'" )
                             .arg( escapeString( cert ) ).arg( error ) );
    return res.next();
}

void DB::addCert( const QString& cert, const QString& error )
{
    execSql( QString( "REPLACE into certificates VALUES('%1', '%2')" )
             .arg( escapeString( cert ) ).arg( error ) );
}

