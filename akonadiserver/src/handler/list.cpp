/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include <QDebug>

#include "akonadi.h"
#include "storagebackend.h"

#include "list.h"
#include "response.h"

using namespace Akonadi;

List::List(): Handler()
{
}


List::~List()
{
}


QByteArray List::constructRealMailboxName( const QByteArray& /*reference*/,
                                           const QByteArray& mailbox )
{
    // FIXME use reference, otherwise use what's selected
    return mailbox;
}


bool List::handleLine(const QByteArray& line )
{
    // parse out the reference name and mailbox name
    int startOfCommand = line.indexOf( ' ' ) + 1;
    int startOfReference = line.indexOf( ' ', startOfCommand ) + 1;
    int startOfMailbox = line.indexOf( ' ', startOfReference ) + 1;
    QByteArray reference = line.mid( startOfReference, line.size() - startOfMailbox -1 );
    QByteArray mailbox = line.right( line.size() - startOfMailbox );
//    qDebug() << "reference:" << reference << "mailbox:" << mailbox << endl;

    mailbox = constructRealMailboxName( reference, mailbox );
    Response response;
    response.setUntagged();
    
    StorageBackend *backend = AkonadiServer::instance()->storageBackend();
    const CollectionList collections = backend->listCollections( mailbox );
    CollectionListIterator it(collections);
    while ( it.hasNext() ) {
        Collection c = it.next();
        QString list( "LIST ");
        list += "(";
        if ( c.isNoSelect() )
            list += "\\Noselect ";
        if ( c.isNoInferiors() )
            list += "\\Noinferiors ";
        list += ")";
        list += "\"/\" \""; // FIXME delimiter
        list += c.identifier();
        list += "\" ";
        response.setString( list.toLatin1() );
        emit responseAvailable( response );
    }
    response.setSuccess();
    response.setTag( tag() );
    response.setString( "List completed" );
    emit responseAvailable( response );
    deleteLater();
    return true;
}

