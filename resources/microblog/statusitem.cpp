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

#include "statusitem.h"
#include <kdebug.h>

#include <QDomElement>

StatusItem::StatusItem(const QByteArray &data)  : m_data( data )
{
    init();
}

StatusItem::~StatusItem()
{
}

void StatusItem::init()
{
    QDomDocument document;
    document.setContent( m_data );
    QDomElement root = document.documentElement();
    QDomNode node = root.firstChild();
    while ( !node.isNull() ) {
        const QString key = node.toElement().tagName();
        if ( key == "user" ) {
            QDomNode node2 = node.firstChild();
            while ( !node2.isNull() ) {
                const QString key2 = node2.toElement().tagName();
                const QString val2 = node2.toElement().text();
                m_status[ "user_" + key2 ] = val2;
                node2 = node2.nextSibling();
            }
        } else {
            const QString value = node.toElement().text();
            m_status[key] = value; 
        }
        node = node.nextSibling();
    }
    //kDebug() << m_status;
}

