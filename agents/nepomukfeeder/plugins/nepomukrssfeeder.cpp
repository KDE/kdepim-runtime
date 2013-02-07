/*
    Copyright (c) 2013 Dan Vrátil <dvratil@redhat.com>

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

#include "nepomukrssfeeder.h"

#include <nrss.h>
#include <nao/tag.h>
#include <nco/contact.h>
#include <nco/emailaddress.h>

#include <Soprano/Vocabulary/NAO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/NCO>
#include <Nepomuk2/Vocabulary/NMM>
#include <Nepomuk2/Vocabulary/NFO>
#include <QFile>
#include <QTemporaryFile>

#include <krss/item.h>
#include <krss/enclosure.h>
#include <krss/feedcollection.h>
#include <krss/person.h>

#include <nepomukfeederutils.h>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KDateTime>

namespace Akonadi {

NepomukRSSFeeder::NepomukRSSFeeder(QObject* parent, const QVariantList &)
: NepomukFeederPlugin(parent)
{
}

NepomukRSSFeeder::~NepomukRSSFeeder()
{
}

void NepomukRSSFeeder::updateItem(const Akonadi::Item& item, Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph)
{
    Q_ASSERT( item.hasPayload() );

    if ( !item.hasPayload<KRss::Item>() ) {
        kWarning() << "Unknown item payload:" << item.mimeType() << "(ID:" << item.id() << ")";
        return;
    }

    KRss::Item rssItem = item.payload<KRss::Item>();
    res.addType( Nepomuk2::Vocabulary::NIE::InformationElement() );
    res.addType( Vocabulary::NRSS::Element() );
    res.addType( Vocabulary::NRSS::Article() );

    NepomukFeederUtils::setIcon( "application-rss+xml", res, graph );

    /* Tags */
    if ( KRss::Item::isImportant( item ) ) {
        addTag( "important", i18n( "Important" ), "mail-mark-important", res, graph );
    }
    if ( KRss::Item::isDeleted( item ) ) {
        addTag( "deleted", i18n( "Deleted" ), "mail-deleted", res, graph );
    }
    res.setProperty( Vocabulary::NRSS::isRead(), KRss::Item::isRead( item ) );

    /* Content */
    res.setProperty( Nepomuk2::Vocabulary::NIE::title(), rssItem.titleAsPlainText() );
    res.setProperty( Soprano::Vocabulary::NAO::prefLabel(), rssItem.titleAsPlainText() );
    if ( !rssItem.description().isEmpty() ) {
        res.setProperty( Nepomuk2::Vocabulary::NIE::description(), rssItem.description() );
    }
    if ( !rssItem.content().isEmpty() ) {
        res.setProperty( Nepomuk2::Vocabulary::NIE::htmlContent(), rssItem.content() );
    }

    Nepomuk2::SimpleResource websiteRes;
    websiteRes.addType( Nepomuk2::Vocabulary::NIE::InformationElement() );
    websiteRes.addType( Nepomuk2::Vocabulary::NIE::DataObject() );
    websiteRes.addType( Nepomuk2::Vocabulary::NFO::Website() );
    websiteRes.setProperty( Nepomuk2::Vocabulary::NIE::url(), rssItem.link() );
    websiteRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), rssItem.title() );
    graph << websiteRes;
    res.addProperty( Vocabulary::NRSS::articleUrl(), websiteRes.uri() );

    res.setProperty( Vocabulary::NRSS::publishTime(), QVariant::fromValue( rssItem.datePublished().dateTime() ) );
    res.setProperty( Vocabulary::NRSS::updateTime(), QVariant::fromValue( rssItem.dateUpdated().dateTime() ) );

    /* Authors */
    Q_FOREACH(const KRss::Person &author, rssItem.authors()) {
        Nepomuk2::SimpleResource contactRes;
        Nepomuk2::NCO::Contact contact( &contactRes );

        if ( !author.name().isEmpty() ) {
            contact.setFullname( author.name() );
        }

        if ( !author.email().isEmpty() ) {
            Nepomuk2::SimpleResource emailRes;
            Nepomuk2::NCO::EmailAddress email( &emailRes );

            email.setEmailAddress( author.email() );

            graph << emailRes;

            contact.setHasEmailAddresses( QList<QUrl>() << emailRes.uri() );
        }

        if ( !author.uri().isEmpty() ) {
            contact.setWebsiteUrls( QList<QUrl>() << author.uri() );
        }

        if ( !author.name().isEmpty() ) {
            contactRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), author.name() );
        } else if ( !author.email().isEmpty() ) {
            contactRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), author.email() );
        }

        graph << contactRes;

        res.addProperty( Nepomuk2::Vocabulary::NCO::creator(), contactRes.uri() );
    }

    /* Enclosures */
    Q_FOREACH(const KRss::Enclosure &rssEnclosure, rssItem.enclosures()) {
        Nepomuk2::SimpleResource enclosureRes;
        enclosureRes.addType( Vocabulary::NRSS::Element() );
        // rssEnclosure.type() is a mimetype of the enclosure */
        if ( rssEnclosure.type().startsWith( "image", Qt::CaseInsensitive ) ) {
            enclosureRes.addType( Vocabulary::NRSS::ImageEnclosure() );
        } else if ( rssEnclosure.type().startsWith( "audio", Qt::CaseInsensitive ) ) {
            enclosureRes.addType( Vocabulary::NRSS::AudioEnclosure() );
            enclosureRes.setProperty( Nepomuk2::Vocabulary::NFO::duration(), rssEnclosure.length() );
        } else if ( rssEnclosure.type().startsWith( "video", Qt::CaseInsensitive ) ) {
            enclosureRes.addType( Vocabulary::NRSS::VideoEnclosure() );
            enclosureRes.setProperty( Nepomuk2::Vocabulary::NFO::duration(), rssEnclosure.length() );
        } else {
            enclosureRes.addType( Vocabulary::NRSS::Enclosure() );
        }

        enclosureRes.setProperty( Nepomuk2::Vocabulary::NIE::title(), rssEnclosure.title() );
        enclosureRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), rssEnclosure.title() );

        Nepomuk2::SimpleResource websiteRes;
        websiteRes.addType( Nepomuk2::Vocabulary::NIE::InformationElement() );
        websiteRes.addType( Nepomuk2::Vocabulary::NIE::DataObject() );
        websiteRes.addType( Nepomuk2::Vocabulary::NFO::WebDataObject() );
        websiteRes.setProperty( Nepomuk2::Vocabulary::NIE::url(), rssEnclosure.url() );
        websiteRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), rssEnclosure.title() );
        graph << websiteRes;
        enclosureRes.addProperty( Vocabulary::NRSS::enclosureUrl(), websiteRes.uri() );


        graph << enclosureRes;
        res.addProperty( Vocabulary::NRSS::enclosure(), enclosureRes.uri() );
    }
}

void NepomukRSSFeeder::addTag(const QString& tagName, const QString& tagLabel, const QString& icon, Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph)
{
    Nepomuk2::SimpleResource tagResource = NepomukFeederUtils::addTag( res, graph, tagName, tagLabel );
    Nepomuk2::NAO::Tag tag( &tagResource );
    if ( !icon.isEmpty() ) {
        NepomukFeederUtils::setIcon( icon, tagResource, graph );
    }
    graph << tagResource;
}


K_PLUGIN_FACTORY(factory, registerPlugin<NepomukRSSFeeder>();)
K_EXPORT_PLUGIN(factory("akonadi_nepomuk_rss_feeder"))


} /* namespace Akonadi */