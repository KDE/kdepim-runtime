/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davutils.h"
#include "davitem.h"
#include "davmanager.h"
#include "davprotocolattribute.h"
#include "davprotocolbase.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>
#include <kcalcore/icalformat.h>
#include <kcalcore/incidence.h>
#include <kcalcore/memorycalendar.h>
#include <klocale.h>

#include <QtCore/QByteArray>
#include <QtCore/QString>

typedef QSharedPointer<KCalCore::Incidence> IncidencePtr;

QDomElement DavUtils::firstChildElementNS( const QDomElement &parent, const QString &namespaceUri, const QString &tagName )
{
  for ( QDomNode child = parent.firstChild(); !child.isNull(); child = child.nextSibling() ) {
    if ( child.isElement() ) {
      const QDomElement elt = child.toElement();
      if ( tagName.isEmpty() || ( elt.tagName() == tagName && elt.namespaceURI() == namespaceUri ) )
        return elt;
    }
  }

  return QDomElement();
}

QDomElement DavUtils::nextSiblingElementNS( const QDomElement &element, const QString &namespaceUri, const QString &tagName )
{
  for ( QDomNode sib = element.nextSibling(); !sib.isNull(); sib = sib.nextSibling() ) {
    if ( sib.isElement() ) {
      const QDomElement elt = sib.toElement();
      if ( tagName.isEmpty() || ( elt.tagName() == tagName && elt.namespaceURI() == namespaceUri ) )
        return elt;
    }
  }

  return QDomElement();
}

DavUtils::Privileges DavUtils::extractPrivileges( const QDomElement &element )
{
  Privileges final = None;
  QDomElement privElement = firstChildElementNS( element, QLatin1String("DAV:"), QLatin1String("privilege"));

  while ( !privElement.isNull() ) {
    QDomElement child = privElement.firstChildElement();

    while ( !child.isNull() ) {
      final |= parsePrivilege( child );
      child = child.nextSiblingElement();
    }

    privElement = DavUtils::nextSiblingElementNS( privElement, QLatin1String("DAV:"), QLatin1String("privilege") );
  }

  return final;
}

DavUtils::Privileges DavUtils::parsePrivilege( const QDomElement &element )
{
  Privileges final = None;

  if ( !element.childNodes().isEmpty() ) {
    // This is an aggregate privilege, parse each of its children
    QDomElement child = element.firstChildElement();
    while ( !child.isNull() ) {
      final |= parsePrivilege( child );
      child = child.nextSiblingElement();
    }
  }
  else {
    // This is a normal privilege
    const QString privname = element.localName();

    if ( privname == QLatin1String("read") )
      final |= DavUtils::Read;
    else if ( privname == QLatin1String("write") )
      final |= DavUtils::Write;
    else if ( privname == QLatin1String("write-properties") )
      final |= DavUtils::WriteProperties;
    else if ( privname == QLatin1String("write-content") )
      final |= DavUtils::WriteContent;
    else if ( privname == QLatin1String("unlock") )
      final |= DavUtils::Unlock;
    else if ( privname == QLatin1String("read-acl") )
      final |= DavUtils::ReadAcl;
    else if ( privname == QLatin1String("read-current-user-privilege-set") )
      final |= DavUtils::ReadCurrentUserPrivilegeSet;
    else if ( privname == QLatin1String("write-acl") )
      final |= DavUtils::WriteAcl;
    else if ( privname == QLatin1String("bind") )
      final |= DavUtils::Bind;
    else if ( privname == QLatin1String("unbind") )
      final |= DavUtils::Unbind;
    else if ( privname == QLatin1String("all") )
      final |= DavUtils::All;
  }

  return final;
}

DavUtils::DavUrl::DavUrl()
  : mProtocol( CalDav )
{
}

DavUtils::DavUrl::DavUrl( const KUrl &url, DavUtils::Protocol protocol )
  : mUrl( url ), mProtocol( protocol )
{
}

void DavUtils::DavUrl::setUrl( const KUrl &url )
{
  mUrl = url;
}

KUrl DavUtils::DavUrl::url() const
{
  return mUrl;
}

void DavUtils::DavUrl::setProtocol( DavUtils::Protocol protocol )
{
  mProtocol = protocol;
}

DavUtils::Protocol DavUtils::DavUrl::protocol() const
{
  return mProtocol;
}

QLatin1String DavUtils::protocolName( DavUtils::Protocol protocol )
{
  QLatin1String protocolName( "" );

  switch( protocol ) {
    case DavUtils::CalDav:
      protocolName = QLatin1String( "CalDav" );
      break;
    case DavUtils::CardDav:
      protocolName = QLatin1String( "CardDav" );
      break;
    case DavUtils::GroupDav:
      protocolName = QLatin1String( "GroupDav" );
      break;
  }

  return protocolName;
}

QString DavUtils::translatedProtocolName( DavUtils::Protocol protocol )
{
  QString protocolName;

  switch( protocol ) {
    case DavUtils::CalDav:
      protocolName = i18n( "CalDav" );
      break;
    case DavUtils::CardDav:
      protocolName = i18n( "CardDav" );
      break;
    case DavUtils::GroupDav:
      protocolName = i18n( "GroupDav" );
      break;
  }

  return protocolName;
}

DavUtils::Protocol DavUtils::protocolByName( const QString &name )
{
  DavUtils::Protocol protocol = DavUtils::CalDav;

  if ( name == QLatin1String("CalDav") ) {
    protocol = DavUtils::CalDav;
  } else if ( name == QLatin1String("CardDav") ) {
    protocol = DavUtils::CardDav;
  } else if ( name == QLatin1String("GroupDav") ) {
    protocol = DavUtils::GroupDav;
  } else {
    kError() << "Unexpected protocol name : " << name;
  }

  return protocol;
}

DavUtils::Protocol DavUtils::protocolByTranslatedName( const QString &name )
{
  DavUtils::Protocol protocol;

  if ( name == i18n( "CalDav" ) ) {
    protocol = DavUtils::CalDav;
  } else if ( name == i18n( "CardDav" ) ) {
    protocol = DavUtils::CardDav;
  } else if ( name == i18n( "GroupDav" ) ) {
    protocol = DavUtils::GroupDav;
  }

  return protocol;
}

QString DavUtils::createUniqueId()
{
  qint64 time = QDateTime::currentMSecsSinceEpoch() / 1000;
  int r = qrand() % 1000;
  QString id = QLatin1String( "R" ) + QString::number( r );
  QString uid = QString::number( time ) + QLatin1String( "." ) + id;
  return uid;
}

DavItem DavUtils::createDavItem( const Akonadi::Item &item, const Akonadi::Collection &collection, const Akonadi::Item::List &dependentItems )
{
  QByteArray rawData;
  QString mimeType;
  KUrl url;
  DavItem davItem;
  const QString basePath = collection.remoteId();

  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();
    const QString fileName = createUniqueId();

    url = KUrl( basePath + fileName + QLatin1String(".vcf") );

    const DavProtocolAttribute *protoAttr = collection.attribute<DavProtocolAttribute>();
    if ( protoAttr ) {
      mimeType =
        DavManager::self()->davProtocol(
          DavUtils::Protocol( protoAttr->davProtocol() ) )->contactsMimeType();
    } else {
      mimeType = KABC::Addressee::mimeType();
    }

    KABC::VCardConverter converter;
    // rawData is already UTF-8
    rawData = converter.exportVCard( contact, KABC::VCardConverter::v3_0 );
  } else if ( item.hasPayload<IncidencePtr>() ) {
    const KCalCore::MemoryCalendar::Ptr calendar( new KCalCore::MemoryCalendar( KDateTime::LocalZone ) );
    calendar->addIncidence( item.payload<IncidencePtr>() );
    foreach ( const Akonadi::Item &dependentItem, dependentItems ) {
      calendar->addIncidence( dependentItem.payload<IncidencePtr>() );
    }

    const QString fileName = createUniqueId();

    url = KUrl( basePath + fileName + QLatin1String(".ics") );
    mimeType = QLatin1String("text/calendar");

    KCalCore::ICalFormat formatter;
    rawData = formatter.toString( calendar, QString() ).toUtf8();
  }

  davItem.setContentType( mimeType );
  davItem.setData( rawData );
  davItem.setUrl( url.prettyUrl() );
  davItem.setEtag( item.remoteRevision() );

  return davItem;
}

bool DavUtils::parseDavData( const DavItem &source, Akonadi::Item &target, Akonadi::Item::List &extraItems )
{
  const QString data = QString::fromUtf8( source.data() );

  if ( target.mimeType() == KABC::Addressee::mimeType() ) {
    KABC::VCardConverter converter;
    const KABC::Addressee contact = converter.parseVCard( source.data() );

    if ( contact.isEmpty() )
      return false;

    target.setPayloadFromData( source.data() );
  } else {
    KCalCore::ICalFormat formatter;
    const KCalCore::MemoryCalendar::Ptr calendar( new KCalCore::MemoryCalendar( KDateTime::LocalZone ) );
    formatter.fromString( calendar, data );
    KCalCore::Incidence::List incidences = calendar->incidences();

    if ( incidences.isEmpty() )
      return false;

    // All items must have the same uid in a single object.
    // Find the main VEVENT (if that's indeed what we have,
    // could be a VTODO or a VJOURNAL but that doesn't matter)
    // and then apply the recurrence exceptions
    IncidencePtr mainIncidence;
    KCalCore::Incidence::List exceptions;

    foreach ( const IncidencePtr &incidence, incidences ) {
      if ( incidence->hasRecurrenceId() ) {
        kDebug() << "Exception found with ID" << incidence->instanceIdentifier();
        exceptions << incidence;
      }
      else {
        mainIncidence = incidence;
      }
    }

    if ( !mainIncidence )
      return false;

    foreach ( const IncidencePtr &exception, exceptions ) {
      if ( exception->status() == KCalCore::Incidence::StatusCanceled ) {
        KDateTime exDateTime = exception->recurrenceId();
        mainIncidence->recurrence()->addExDateTime( exDateTime );
      }
      else {
        // The exception remote id will contain a fragment pointing to
        // its instance identifier to distinguish it from the main
        // event.
        QString rid = target.remoteId() + QLatin1String( "#" ) + exception->instanceIdentifier();
        kDebug() << "Extra incidence at" << rid;
        Akonadi::Item extraItem = target;
        extraItem.setRemoteId( rid );
        extraItem.setRemoteRevision( source.etag() );
        extraItem.setMimeType( exception->mimeType() );
        extraItem.setPayload<IncidencePtr>( exception );
        extraItems << extraItem;
      }
    }

    target.setPayload<IncidencePtr>( mainIncidence );
    // fix mime type for CalDAV collections
    target.setMimeType( mainIncidence->mimeType() );

    /*
    foreach ( const IncidencePtr &incidence, incidences ) {
      QString rid = item.remoteId() + QLatin1String( "#" ) + incidence->instanceIdentifier();
      Akonadi::Item extraItem = item;
      extraItem.setRemoteId( rid );
      extraItem.setRemoteRevision( davItem.etag() );
      extraItem.setMimeType( incidence->mimeType() );
      extraItem.setPayload<IncidencePtr>( incidence );
      items << extraItem;
    }
    */
  }

  return true;
}
