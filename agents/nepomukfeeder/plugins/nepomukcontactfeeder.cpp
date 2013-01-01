/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2011 Martin Klapetek <martin.klapetek@gmail.com>
                  2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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


#include "nepomukcontactfeeder.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <Nepomuk2/Vocabulary/NCO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Soprano/Vocabulary/NAO>

#include <KUrl>
#include <KStandardDirs>
#include <QDir>

// ontology includes
#include <nepomuk/nfo.h>
#include <Soprano/Vocabulary/NAO>
#include <Nepomuk/Vocabulary/NIE>
#include <nco/affiliation.h>
#include <nco/audioimaccount.h>
#include <nco/bbsnumber.h>
#include <nco/carphonenumber.h>
#include <nco/cellphonenumber.h>
#include <nco/contact.h>
#include <nco/contactgroup.h>
#include <nco/contactlist.h>
#include <nco/domesticdeliveryaddress.h>
#include <nco/emailaddress.h>
#include <nco/faxnumber.h>
#include <nco/imaccount.h>
#include <nco/internationaldeliveryaddress.h>
#include <nco/isdnnumber.h>
#include <nco/messagingnumber.h>
#include <nco/modemnumber.h>
#include <nco/organizationcontact.h>
#include <nco/pagernumber.h>
#include <nco/parceldeliveryaddress.h>
#include <nco/pcsnumber.h>
#include <nco/personcontact.h>
#include <nco/phonenumber.h>
#include <nco/postaladdress.h>
#include <nco/videoimaccount.h>
#include <nco/videotelephonenumber.h>
#include <nco/voicephonenumber.h>

#include <KDebug>

#include <nepomukfeederutils.h>

#include <kexportplugin.h>
#include <kpluginfactory.h>

using namespace Nepomuk2;


namespace Akonadi {

namespace {
  inline QStringList listFromString( const QString& s ) {
    if ( s.isEmpty() )
      return QStringList();
    else
      return QStringList( s );
  }
}

void NepomukContactFeeder::updateItem(const Akonadi::Item& item, Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph)
{
  //kDebug() << item.id();
  Q_ASSERT( item.hasPayload() );
  if ( item.hasPayload<KABC::Addressee>() )
    updateContactItem( item, res, graph );
  else if ( item.hasPayload<KABC::ContactGroup>() )
    updateGroupItem( item, res, graph );
  else
    kWarning() << "Got item without known payload. Mimetype:" << item.mimeType()
    << "Id:" << item.id();
}

void NepomukContactFeeder::updateContactItem( const Akonadi::Item &item, Nepomuk2::SimpleResource &res, Nepomuk2::SimpleResourceGraph &graph )
{
    res.addType( Nepomuk2::Vocabulary::NCO::Contact() );

    //NepomukFeederUtils::setIcon( "view-pim-contacts", res, graph );

    Nepomuk2::NCO::Contact contact( &res );
    Nepomuk2::NCO::PersonContact person( &res );

    const KABC::Addressee addressee = item.payload<KABC::Addressee>();

    if ( !addressee.photo().isEmpty() ) {
        const KStandardDirs ksd;
        const QDir storeDir( QDir::toNativeSeparators( ksd.localxdgdatadir().append( "/nepomuk-contact-images/" ) ) );

        if ( !storeDir.exists() ) {
            storeDir.mkpath( storeDir.absolutePath() );
        }

        const QString filePath = storeDir.absolutePath().append( "/%1.png" ).arg( addressee.uid() );
        bool imageSaved = addressee.photo().data().save( filePath, "PNG" );

        if ( imageSaved ) {
            KUrl fileUrl( filePath );
            fileUrl.setProtocol( "file" );

            res.addProperty( Nepomuk2::Vocabulary::NCO::photo(), fileUrl.url() );
        }
    }

    if ( !addressee.formattedName().isEmpty() ) {
        contact.setFullname( addressee.formattedName() );
        res.addProperty( Soprano::Vocabulary::NAO::prefLabel(), addressee.formattedName() );
    } else {
        contact.setFullname( addressee.assembledName() );
        res.addProperty( Soprano::Vocabulary::NAO::prefLabel(), addressee.assembledName() );
    }

    if ( !addressee.givenName().isEmpty() )
        person.setNameGiven( addressee.givenName() );

    if ( !addressee.additionalName().isEmpty() )
        person.setNameAdditionals( listFromString( addressee.additionalName() ) );

    if ( !addressee.familyName().isEmpty() )
        person.setNameFamily( addressee.familyName() );

    if ( !addressee.prefix().isEmpty() )
        person.setNameHonorificPrefixs( listFromString( addressee.prefix() ) );

    if ( !addressee.suffix().isEmpty() )
        person.setNameHonorificSuffixs( listFromString( addressee.suffix() ) );

    const KABC::Geo geo = addressee.geo();
    if ( geo.isValid() ) {
        //FIXME we currently don't have ontologies for Location
        //       Nepomuk2::SimpleResource locRes;
        //       QString geoString;
        //       geoString.sprintf( "%.6f;%.6f", geo.latitude(), geo.longitude() );// make it better
        //       contact.setHasLocation( locRes.uri() );
    }

    // keys
    // sounds
    // logos
    // photos
    if ( !addressee.note().isEmpty() )
        contact.setNotes( listFromString( addressee.note() ) );

    if ( !addressee.nickName().isEmpty() )
        contact.setNicknames( listFromString( addressee.nickName() ) );

    contact.setContactUID( addressee.uid() ); // never empty

    if ( !addressee.name().isEmpty() )
        contact.setFullname( addressee.name() );

    if ( addressee.birthday().date().isValid() )
        contact.setBirthDate( addressee.birthday().date() );

//FIXME nco:websiteUrl has a range of rdfs:resource and not xsd:string. Maybe use rdfs:literal?
//     if ( addressee.url().isValid() ) {
//         KUrl url = addressee.url();
// 
//         // Nepomuk doesn't like URLs without a protocol
//         if ( url.protocol().isEmpty() )
//             url.setProtocol( "http" );
// 
//         contact.addWebsiteUrl( url.url() );
//     }

    Nepomuk2::SimpleResource affiliationRes;
    Nepomuk2::NCO::Affiliation affiliation( &affiliationRes );

    if ( !addressee.organization().isEmpty() ) {
        Nepomuk2::SimpleResource organizationRes;
        Nepomuk2::NCO::OrganizationContact orgContact( &organizationRes );
        orgContact.setFullname( addressee.organization() );

        affiliation.setOrg( organizationRes.uri() );

        graph << organizationRes;
    }
    if ( !addressee.role().isEmpty() ) {
        affiliation.setRoles( listFromString( addressee.role() ) );
    }
    if ( !addressee.title().isEmpty() ) {
        affiliation.setTitle( addressee.title() );
    }
    if ( !addressee.department().isEmpty() ) {
        affiliation.setDepartments( listFromString( addressee.department() ) );
    }

    // phone numbers
    const KABC::PhoneNumber::List phoneNumbers = addressee.phoneNumbers();
    const int countOfPhoneNumber( phoneNumbers.count() );
    for ( int i = 0; i < countOfPhoneNumber; ++i ) {
        Nepomuk2::SimpleResource affiliationPhoneRes;
        Nepomuk2::SimpleResource phoneRes;
        if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Bbs ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::BbsNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::BbsNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Car ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::CarPhoneNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::CarPhoneNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Cell ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::CellPhoneNumber number( &affiliationPhoneRes );
                //FIXME: this could really use some better way, but it depends on autogenerated code from ontologies
                //       which needs fixing first
                number.Nepomuk2::NCO::MessagingNumber::addPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::CellPhoneNumber number( &phoneRes );
                //FIXME: this could really use some better way, but it depends on autogenerated code from ontologies
                //       which needs fixing first
                number.Nepomuk2::NCO::MessagingNumber::addPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Fax ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::FaxNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::FaxNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Isdn ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::IsdnNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::IsdnNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Msg ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::MessagingNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::MessagingNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Modem ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::ModemNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::ModemNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Pager ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::PagerNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::PagerNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Pcs ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::PcsNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::PcsNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Video ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::VideoTelephoneNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::VideoTelephoneNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Voice ) {
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::VoicePhoneNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::VoicePhoneNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        } else { // matches Home and Work
            if ( phoneNumbers[ i ].type() & KABC::PhoneNumber::Work ) {
                Nepomuk2::NCO::PhoneNumber number( &affiliationPhoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            } else {
                Nepomuk2::NCO::PhoneNumber number( &phoneRes );
                number.setPhoneNumber( phoneNumbers[ i ].number() );
            }
        }

        if ( affiliationPhoneRes.isValid() ) {
            affiliation.addHasPhoneNumber( affiliationPhoneRes.uri() );
            graph << affiliationPhoneRes;
        }
        if ( phoneRes.isValid() ) {
            contact.addHasPhoneNumber( phoneRes.uri() );
            graph << phoneRes;
        }
    }

    if ( affiliationRes.isValid() ) {
        person.setHasAffiliations( QList<QUrl>() << affiliationRes.uri() );
        graph << affiliationRes;
    }

    // im accounts

    // email addresses
    const QStringList emails = addressee.emails();
    for ( int i = 0; i < emails.count(); ++i ) {
        Nepomuk2::SimpleResource emailRes;
        Nepomuk2::NCO::EmailAddress email( &emailRes );
        email.setEmailAddress( emails[ i ].toLower() );

        contact.addHasEmailAddress( emailRes.uri() );
        graph << emailRes;
    }

    // addresses
    const KABC::Address::List addresses = addressee.addresses();
    const int numberOfAddresses( addresses.count() );
    for ( int i = 0; i < numberOfAddresses; ++i ) {
        Nepomuk2::SimpleResource postalRes;
        Nepomuk2::NCO::PostalAddress address( &postalRes );
        address.setStreetAddress( addresses[ i ].street() );
        if ( !addresses[ i ].postalCode().isEmpty() )
            address.setPostalcode( addresses[ i ].postalCode() );
        if ( !addresses[ i ].locality().isEmpty() )
            address.setLocality( addresses[ i ].locality() );
        if ( !addresses[ i ].region().isEmpty() )
            address.setRegion( addresses[ i ].region() );
        if ( !addresses[ i ].postOfficeBox().isEmpty() )
            address.setPobox( addresses[ i ].postOfficeBox() );
        if ( !addresses[ i ].country().isEmpty() )
            address.setCountry( addresses[ i ].country() );
        address.setExtendedAddress( addresses[ i ].extended() );

        contact.addHasPostalAddress( postalRes.uri() );
        graph << postalRes;
    }

    NepomukFeederUtils::tagsFromCategories( addressee.categories(), res, graph );
}

void NepomukContactFeeder::updateGroupItem( const Akonadi::Item &item, Nepomuk2::SimpleResource &res, Nepomuk2::SimpleResourceGraph &graph )
{
    // create the contact group with the graph reference
    Nepomuk2::NCO::ContactGroup group( &res );

    const KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();

    group.setContactGroupName( contactGroup.name() );

    res.addProperty( Soprano::Vocabulary::NAO::prefLabel(), contactGroup.name() );

    for ( uint i = 0; i < contactGroup.contactReferenceCount(); ++i ) {
        const Akonadi::Item contactItem( contactGroup.contactReference( i ).uid().toLongLong() );

        Nepomuk2::SimpleResource personRes;
        Nepomuk2::NCO::PersonContact person( &personRes );
        person.addBelongsToGroup( res.uri() );

        graph << personRes;
    }
}


K_PLUGIN_FACTORY(factory, registerPlugin<NepomukContactFeeder>();)
K_EXPORT_PLUGIN(factory("akonadi_nepomuk_contact_feeder"))

}

#include "nepomukcontactfeeder.moc"
