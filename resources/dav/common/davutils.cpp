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
  QDomElement privElement = firstChildElementNS( element, "DAV:", "privilege" );

  while ( !privElement.isNull() ) {
    QDomElement child = privElement.firstChildElement();

    while ( !child.isNull() ) {
      final |= parsePrivilege( child );
      child = child.nextSiblingElement();
    }

    privElement = DavUtils::nextSiblingElementNS( privElement, "DAV:", "privilege" );
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

    if ( privname == "read" )
      final |= DavUtils::Read;
    else if ( privname == "write" )
      final |= DavUtils::Write;
    else if ( privname == "write-properties" )
      final |= DavUtils::WriteProperties;
    else if ( privname == "write-content" )
      final |= DavUtils::WriteContent;
    else if ( privname == "unlock" )
      final |= DavUtils::Unlock;
    else if ( privname == "read-acl" )
      final |= DavUtils::ReadAcl;
    else if ( privname == "read-current-user-privilege-set" )
      final |= DavUtils::ReadCurrentUserPrivilegeSet;
    else if ( privname == "write-acl" )
      final |= DavUtils::WriteAcl;
    else if ( privname == "bind" )
      final |= DavUtils::Bind;
    else if ( privname == "unbind" )
      final |= DavUtils::Unbind;
    else if ( privname == "all" )
      final |= DavUtils::All;
  }

  return final;
}

DavUtils::DavUrl::DavUrl()
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
  DavUtils::Protocol protocol;

  if ( name == "CalDav" ) {
    protocol = DavUtils::CalDav;
  } else if ( name == "CardDav" ) {
    protocol = DavUtils::CardDav;
  } else if ( name == "GroupDav" ) {
    protocol = DavUtils::GroupDav;
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

DavItem DavUtils::createDavItem( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  QByteArray rawData;
  QString mimeType;
  KUrl url;
  DavItem davItem;
  const QString basePath = collection.remoteId();

  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();

    const QString fileName = contact.uid();
    if ( fileName.isEmpty() ) {
      kError() << "Invalid contact uid";
      return davItem;
    }

    url = KUrl( basePath + fileName + ".vcf" );

    const DavProtocolAttribute *protoAttr = collection.attribute<DavProtocolAttribute>();
    if ( protoAttr ) {
      mimeType =
        DavManager::self()->davProtocol(
          DavUtils::Protocol( protoAttr->davProtocol() ) )->contactsMimeType();
    } else {
      mimeType = KABC::Addressee::mimeType();
    }

    KABC::VCardConverter converter;
    QByteArray localData = converter.exportVCard( contact, KABC::VCardConverter::v3_0 );
    // Convert to UTF-8
    rawData = QString::fromLocal8Bit( localData.data() ).toUtf8();
  } else if ( item.hasPayload<IncidencePtr>() ) {
    const IncidencePtr ptr = item.payload<IncidencePtr>();

    const QString fileName = ptr->uid();
    if ( fileName.isEmpty() ) {
      kError() << "Invalid incidence uid";
      return davItem;
    }

    url = KUrl( basePath + fileName + ".ics" );
    mimeType = "text/calendar";

    KCalCore::ICalFormat formatter;
    rawData = formatter.toICalString( ptr ).toUtf8();
  }

  davItem.setContentType( mimeType );
  davItem.setData( rawData );
  davItem.setUrl( url.prettyUrl() );
  davItem.setEtag( item.remoteRevision() );

  return davItem;
}
