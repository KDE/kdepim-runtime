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
#include <QtCore/QDebug>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "imapparser.h"

#include "list.h"
#include "response.h"

using namespace Akonadi;

List::List(): Handler()
{
}


List::~List()
{
}


bool List::handleLine(const QByteArray& line )
{
    // parse out the reference name and mailbox name
    int pos = line.indexOf( ' ' ) + 1; // skip tag
    pos = line.indexOf( ' ', pos ) + 1; // skip command
    QString reference;
    pos = ImapParser::parseString( line, reference, pos );
    QString mailbox;
    ImapParser::parseString( line, mailbox, pos );

//     qDebug() << "reference:" << reference << "mailbox:" << mailbox << "::" << endl;

    Response response;
    response.setUntagged();

    if ( mailbox.isEmpty() ) { // special case of asking for the delimiter
        response.setString( "LIST (\\Noselect) \"/\" \"\"" );
        emit responseAvailable( response );
    } else {
        CollectionList collections = listCollections( reference, mailbox );
        if ( !collections.isValid() ) {
          return failureResponse( "Unable to find collection" );
        }

        // TODO: Create a response with the mime types for the listed folder:
        // * FLAGS (\MimeTypes[text/calendar/inode/directory])

        CollectionListIterator it(collections);
        while ( it.hasNext() ) {
            Collection c = it.next();
            QByteArray list( "LIST ");
            list += '(';
            bool first = true;
            if ( c.isNoSelect() ) {
                list += "\\Noselect";
                first = false;
            }
            if ( c.isNoInferiors() ) {
                if ( !first ) list += ' ';
                list += "\\Noinferiors";
                first = false;
            }
            const QByteArray supportedMimeTypes = c.getMimeTypes();
            if ( !supportedMimeTypes.isEmpty() ) {
                if ( !first ) list += ' ';
                list += "\\MimeTypes[" + c.getMimeTypes() + ']';
            }
            list += ") ";
            list += "\"/\" \""; // FIXME delimiter
            list += c.identifier().toUtf8();
            list += "\"";
            response.setString( list );
            emit responseAvailable( response );
        }
    }

    response.setSuccess();
    response.setTag( tag() );
    response.setString( "List completed" );
    emit responseAvailable( response );
    deleteLater();
    return true;
}

CollectionList List::listCollections( const QString & prefix,
                                      const QString & mailboxPattern )
{
  CollectionList result;

  if ( mailboxPattern.isEmpty() )
    return result;

  DataStore *db = connection()->storageBackend();
  const QString locationDelimiter = db->locationDelimiter();

  // normalize path and pattern
  QString sanitizedPattern( mailboxPattern );
  QString fullPrefix( prefix );
  const bool hasPercent = mailboxPattern.contains( QLatin1Char('%') );
  const bool hasStar = mailboxPattern.contains( QLatin1Char('*') );
  const int endOfPath = mailboxPattern.lastIndexOf( locationDelimiter ) + 1;

  Resource resource;
  if ( fullPrefix.startsWith( QLatin1Char('#') ) ) {
    int endOfRes = fullPrefix.indexOf( locationDelimiter );
    QString resourceName;
    if ( endOfRes < 0 ) {
      resourceName = fullPrefix.mid( 1 );
      fullPrefix = QString();
    } else {
      resourceName = fullPrefix.mid( 1, endOfRes - 1 );
      fullPrefix = fullPrefix.right( fullPrefix.length() - endOfRes );
    }

    qDebug() << "listCollections()" << resourceName;
    resource = Resource::retrieveByName( resourceName );
    qDebug() << "resource.isValid()" << resource.isValid();
    if ( !resource.isValid() ) {
      result.setValid( false );
      return result;
    }
  }

  if ( !mailboxPattern.startsWith( locationDelimiter ) && fullPrefix != locationDelimiter )
    fullPrefix += locationDelimiter;
  fullPrefix += mailboxPattern.left( endOfPath );

  if ( hasPercent )
    sanitizedPattern = QLatin1String("%");
  else if ( hasStar )
    sanitizedPattern = QLatin1String("*");
  else
    sanitizedPattern = mailboxPattern.mid( endOfPath );

  qDebug() << "Resource: " << resource.name() << " fullPrefix: " << fullPrefix << " pattern: " << sanitizedPattern;

  if ( !fullPrefix.isEmpty() ) {
    result.setValid( false );
  }

  const QList<Location> locations = db->listLocations( resource );
  foreach( Location l, locations ) {
    const QString location = locationDelimiter + l.name();
#if 0
    qDebug() << "Location: " << location << " l: " << l << " prefix: " << fullPrefix;
#endif
    const bool atFirstLevel =
      location.lastIndexOf( locationDelimiter ) == fullPrefix.lastIndexOf( locationDelimiter );
    if ( location.startsWith( fullPrefix ) ) {
      if ( hasStar || ( hasPercent && atFirstLevel ) ||
           location == fullPrefix + sanitizedPattern ) {
        Collection c( location.right( location.size() -1 ) );
        c.setMimeTypes( MimeType::joinByName<MimeType>( l.mimeTypes(), QLatin1String(",") ).toLatin1() );
        result.append( c );
      }
    }
    // Check, if requested folder has been found to distinguish between
    // non-existant folder and empty folder.
    if ( location + locationDelimiter == fullPrefix || fullPrefix == locationDelimiter )
      result.setValid( true );
  }

  // list queries (only in global namespace)
  if ( !resource.isValid() ) {
    if ( fullPrefix == locationDelimiter ) {
      CollectionList persistenSearches = db->listPersistentSearches();
      if ( !persistenSearches.isEmpty() )
        result.append( Collection( QLatin1String("Search") ) );
      result.setValid( true );
    }
    if ( fullPrefix == QLatin1String("/Search/")  || (fullPrefix == locationDelimiter && hasStar) ) {
      result += db->listPersistentSearches();
      result.setValid( true );
    }
  }

  return result;
}
