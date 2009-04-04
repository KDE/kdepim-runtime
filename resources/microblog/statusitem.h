/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#ifndef __STATUSITEM_H__
#define __STATUSITEM_H__

#include <QSharedDataPointer>
#include <QByteArray>
#include <QHash>
#include <QDateTime>

/** 
 * @class StatusItem
 *
 * This class is a representation of one Dent or Tweet. It is filled with 
 * xml which the REST API from ident.ca or Teitter and parses it and gives
 * back the values. There are also some convenience functions like 
 */
class StatusItem
{
public:
    /** Constructor */
    StatusItem();

    /** Constructor which takes the XML as argument. The data is parsed 
      * instantly, so the other methods of the class are instantly usuable
      */
    StatusItem( const QByteArray& );

    /** Copy constructor */
    StatusItem( const StatusItem& );

    /** Destructor */
    ~StatusItem();

    /** Coparisation operator */
    StatusItem operator=( const StatusItem& );

    /** The call to set the XML data. After this, the data is parsed
      * instantly, so the other methods of the class are instantly usuable
      */
    void setData( const QByteArray& );

    /** Returns the unique id as given by the service */
    qlonglong id() const;

    /** Returns the value of a certain key. The keys can be obtained via
     *  keys()
     */
    QString value( const QString& ) const;

    /** Gives the text of the tweet or dent. The result is HTML where links
     *  are hrefs and smileys are images.
     */
    QString text() const;

    /** Returns all the keys available */
    QStringList keys() const;

    /** Returns the date of the dent or tweet */
    QDateTime date() const;

    /** Gives the raw xml data of the tweet or dent */
    QByteArray data() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

#endif
