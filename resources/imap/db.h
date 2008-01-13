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

/** @file db.h */

#ifndef DB_H
#define DB_H

#include <QStringList>
#include <Q3ValueList>
#include <QSqlDatabase>
#include <QSqlQuery>

typedef Q3ValueList<int>     IntList;
typedef Q3ValueList<qint64> LLongList;

/**
 * This class is responsible for the communication
 * with the sqlite database.
 */
class DB
{
public:

    /**
     * Constructor
     */
    DB();

    /**
     * Destructor
     */
    ~DB();

    /**
     * Makes a connection to the database and makes sure all tables
     * are available.
     * @param path The database to open
     */
    void setDBPath( const QString& path );
    QSqlDatabase db() {
        return m_db;
    } ;

    /**
     * This will execute a given SQL statement to the database.
     * @param sql The SQL statement
     * @param values This will be filled with the result of the SQL statement
     * @param debug If true, it will output the SQL statement
     * @return It will return if the execution of the statement was succesfull
     */
    QSqlQuery execSql( const QString& sql );

    bool hasCert( const QString&, const QString& );
    void addCert( const QString&, const QString& );

private:

    /**
     * Checks the available tables and creates them if they are not
     * available.
     */
    void initDB();

    /**
     * Escapes text fields. This is needed for all queries to the database
     * which happens with an argument which is a text field. It makes sure
     * a ' is replaced with '', as this is needed for sqlite.
     * @param str String to escape
     * @return The escaped string
     */
    QString escapeString( const QString& str ) const;

    QSqlDatabase m_db;
};

#endif
