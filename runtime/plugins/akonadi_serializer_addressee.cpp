/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#include "akonadi_serializer_addressee.h"

#include <akonadi/abstractdifferencesreporter.h>
#include <akonadi/item.h>
#include <akonadi/kabc/contactparts.h>

#include <kabc/addressee.h>
#include <klocale.h>

#include <QtCore/qplugin.h>

using namespace Akonadi;

//// ItemSerializerPlugin interface

bool SerializerPluginAddressee::deserialize( Item& item, const QByteArray& label, QIODevice& data, int version )
{
    Q_UNUSED( version );

    KABC::Addressee addr;
    if ( label == Item::FullPayload ) {
        addr = m_converter.parseVCard( data.readAll() );
    } else if ( label == Akonadi::ContactPart::Standard ) {
        addr = m_converter.parseVCard( data.readAll() );

        // remove pictures and sound
        addr.setPhoto( KABC::Picture() );
        addr.setLogo( KABC::Picture() );
        addr.setSound( KABC::Sound() );
    } else if ( label == Akonadi::ContactPart::Lookup ) {
        const KABC::Addressee temp = m_converter.parseVCard( data.readAll() );

        // copy only uid, name and email addresses
        addr.setUid( temp.uid() );
        addr.setPrefix( temp.prefix() );
        addr.setGivenName( temp.givenName() );
        addr.setAdditionalName( temp.additionalName() );
        addr.setFamilyName( temp.familyName() );
        addr.setSuffix( temp.suffix() );
        addr.setEmails( temp.emails() );
    } else {
        return false;
    }

    if ( !addr.isEmpty() ) {
        item.setPayload<KABC::Addressee>( addr );
    } else {
        kWarning( 5261 ) << "Empty addressee object!";
    }

    return true;
}

void SerializerPluginAddressee::serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version )
{
    Q_UNUSED( version );

    if ( label != Item::FullPayload && label != Akonadi::ContactPart::Standard && label != Akonadi::ContactPart::Lookup )
        return;

    if ( !item.hasPayload<KABC::Addressee>() )
        return;

    KABC::Addressee addr, temp;

    temp = item.payload<KABC::Addressee>();

    if ( label == Item::FullPayload ) {
        addr = temp;
    } else if ( label == Akonadi::ContactPart::Standard ) {
        addr = temp;

        // remove pictures and sound
        addr.setPhoto( KABC::Picture() );
        addr.setLogo( KABC::Picture() );
        addr.setSound( KABC::Sound() );
    } else if ( label == Akonadi::ContactPart::Lookup ) {
        // copy only uid, name and email addresses
        addr.setUid( temp.uid() );
        addr.setPrefix( temp.prefix() );
        addr.setGivenName( temp.givenName() );
        addr.setAdditionalName( temp.additionalName() );
        addr.setFamilyName( temp.familyName() );
        addr.setSuffix( temp.suffix() );
        addr.setEmails( temp.emails() );
    }

    data.write( m_converter.createVCard( addr ) );
}

//// DifferencesAlgorithmInterface interface

static bool compareString( const QString &left, const QString &right )
{
  if ( left.isEmpty() && right.isEmpty() )
    return true;
  else
    return left == right;
}

static QString toString( const KABC::PhoneNumber &phoneNumber )
{
  return phoneNumber.number();
}

static QString toString( const KABC::Address &address )
{
  return address.toString();
}

static QString toString( const QString &value )
{
  return value;
}

template <class T>
static void compareList( Akonadi::AbstractDifferencesReporter *reporter, const QString &id, const QList<T> &left, const QList<T> &right )
{
  for ( int i = 0; i < left.count(); ++i ) {
    if ( !right.contains( left[ i ] )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalLeftMode, id, toString( left[ i ] ), QString() );
  }

  for ( int i = 0; i < right.count(); ++i ) {
    if ( !left.contains( right[ i ] )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalRightMode, id, QString(), toString( right[ i ] ) );
  }
}

void SerializerPluginAddressee::compare( Akonadi::AbstractDifferencesReporter *reporter,
                                         const Akonadi::Item &leftItem,
                                         const Akonadi::Item &rightItem )
{
  Q_ASSERT( reporter );
  Q_ASSERT( leftItem.hasPayload<KABC::Addressee>() );
  Q_ASSERT( rightItem.hasPayload<KABC::Addressee>() );

  reporter->setLeftPropertyValueTitle( i18n( "Changed Contact" ) );
  reporter->setRightPropertyValueTitle( i18n( "Conflicting Contact" ) );

  const KABC::Addressee leftContact = leftItem.payload<KABC::Addressee>();
  const KABC::Addressee rightContact = rightItem.payload<KABC::Addressee>();

  if ( !compareString( leftContact.uid(), rightContact.uid() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::uidLabel(),
                            leftContact.uid(), rightContact.uid() );

  if ( !compareString( leftContact.name(), rightContact.name() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::nameLabel(),
                            leftContact.name(), rightContact.name() );

  if ( !compareString( leftContact.formattedName(), rightContact.formattedName() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::formattedNameLabel(),
                            leftContact.formattedName(), rightContact.formattedName() );

  if ( !compareString( leftContact.familyName(), rightContact.familyName() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::familyNameLabel(),
                            leftContact.familyName(), rightContact.familyName() );

  if ( !compareString( leftContact.givenName(), rightContact.givenName() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::givenNameLabel(),
                            leftContact.givenName(), rightContact.givenName() );

  if ( !compareString( leftContact.additionalName(), rightContact.additionalName() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::additionalNameLabel(),
                            leftContact.additionalName(), rightContact.additionalName() );

  if ( !compareString( leftContact.prefix(), rightContact.prefix() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::prefixLabel(),
                            leftContact.prefix(), rightContact.prefix() );

  if ( !compareString( leftContact.suffix(), rightContact.suffix() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::suffixLabel(),
                            leftContact.suffix(), rightContact.suffix() );

  if ( !compareString( leftContact.nickName(), rightContact.nickName() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::nickNameLabel(),
                            leftContact.nickName(), rightContact.nickName() );

  if ( leftContact.birthday() != rightContact.birthday() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::birthdayLabel(),
                            leftContact.birthday().toString(), rightContact.birthday().toString() );

  if ( !compareString( leftContact.mailer(), rightContact.mailer() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::mailerLabel(),
                            leftContact.mailer(), rightContact.mailer() );

  if ( leftContact.timeZone() != rightContact.timeZone() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::timeZoneLabel(),
                            leftContact.timeZone().toString(), rightContact.timeZone().toString() );

  if ( leftContact.geo() != rightContact.geo() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::geoLabel(),
                            leftContact.geo().toString(), rightContact.geo().toString() );

  if ( !compareString( leftContact.title(), rightContact.title() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::titleLabel(),
                            leftContact.title(), rightContact.title() );

  if ( !compareString( leftContact.role(), rightContact.role() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::roleLabel(),
                            leftContact.role(), rightContact.role() );

  if ( !compareString( leftContact.organization(), rightContact.organization() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::organizationLabel(),
                            leftContact.organization(), rightContact.organization() );

  if ( !compareString( leftContact.note(), rightContact.note() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::noteLabel(),
                            leftContact.note(), rightContact.note() );

  if ( !compareString( leftContact.productId(), rightContact.productId() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::productIdLabel(),
                            leftContact.productId(), rightContact.productId() );

  if ( !compareString( leftContact.sortString(), rightContact.sortString() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::sortStringLabel(),
                            leftContact.sortString(), rightContact.sortString() );

  if ( leftContact.secrecy() != rightContact.secrecy() ) {
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::secrecyLabel(),
                            leftContact.secrecy().toString(), rightContact.secrecy().toString() );
  }

  if ( leftContact.url() != rightContact.url() )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, KABC::Addressee::urlLabel(),
                            leftContact.url().prettyUrl(), rightContact.url().prettyUrl() );

  compareList( reporter, i18n( "Emails" ), leftContact.emails(), rightContact.emails() );
  compareList( reporter, i18n( "Phone Numbers" ), leftContact.phoneNumbers(), rightContact.phoneNumbers() );
  compareList( reporter, i18n( "Addresses" ), leftContact.addresses(), rightContact.addresses() );

  //TODO: logo/photo/custom entries
}

Q_EXPORT_PLUGIN2( akonadi_serializer_addressee, Akonadi::SerializerPluginAddressee )

#include "akonadi_serializer_addressee.moc"
