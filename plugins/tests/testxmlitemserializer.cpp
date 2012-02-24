/*
 * This file is part of the krss library
 *
 * Copyright (C) 2007 Frank Osterfeld <osterfeld@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "testxmlitemserializer.h"

#include <krss/enclosure.h>
#include <krss/person.h>
#include <krss/category.h>
#include <krss/item.h>
#include <krss/rssitem.h>
#include "rssitemserializer.h"

#include <KDateTime>

#include <qtest_kde.h>

#include <QDebug>
#include <QByteArray>
#include <QVariant>

using namespace KRss;

namespace {

void printItem( const RssItem& item )
{
    QByteArray ba;
    RssItemSerializer::serialize( item, ba );
    qDebug() << ba.size() << ba;
}

void testItem( const RssItem& item )
{
    const bool headersLoaded = item.headersLoaded();
    const bool contentLoaded = item.contentLoaded();
    QByteArray ba;
    RssItemSerializer::serialize( item, ba );
    RssItem deserialized;
    const bool success = RssItemSerializer::deserialize( deserialized, ba );
    QVERIFY2( success, "Deserialization failed" );
    deserialized.setContentLoaded( contentLoaded );
    deserialized.setHeadersLoaded( headersLoaded );
    if ( item != deserialized )
    {
        printItem( item );
        printItem( deserialized );
    }
    QCOMPARE( item, deserialized );
}

}

void TestXmlItemSerializer::testEmptyItem()
{
    ::testItem( RssItem() );
}

void TestXmlItemSerializer::testDates()
{
    RssItem item;
    const KDateTime updated = KDateTime::currentLocalDateTime();
    const KDateTime published = updated.addDays( -4 );
    item.setDatePublished( published );
    item.setDateUpdated( updated );
    ::testItem( item );
}

void TestXmlItemSerializer::testSimpleItems()
{
    RssItem item;
    item.setTitle( QLatin1String("Some title") );
    item.setDescription( QLatin1String("Some description") );
    item.setLink( QLatin1String("http://akregator.kde.org") );
    item.setContent( QLatin1String("Content makes the world go round") );
    item.setGuid( QLatin1String("http://uniqueid") );
    item.setGuidIsHash( true );
    item.setLanguage( QLatin1String("en") );
    item.setHash( 5 );
    ::testItem( item );
    item.setHash( 0 );
    ::testItem( item );
}

void TestXmlItemSerializer::testStatus()
{
#if 0
    RssItem item;
    item.setStatus( Read );
    ::testItem( item );
    item.setStatus( New );
    ::testItem( item );
    item.setStatus( Important );
    ::testItem( item );
    item.setStatus( Read | Important );
    ::testItem( item );
#endif
}

void TestXmlItemSerializer::testCustomProperties()
{
    RssItem item;
    item.setGuid( QLatin1String("http://uniqueid") );
    item.setTitle( QLatin1String("Some title") );
    item.setDescription( QLatin1String("Some description") );
    item.setLink( QLatin1String("http://akregator.kde.org") );
    item.setCustomProperty( QLatin1String("foo"), QLatin1String("bar") );
    item.setCustomProperty( QLatin1String("bar"), QLatin1String("foo") );
    item.setCustomProperty( QLatin1String("foobar"), QString() );
    ::testItem( item );
}

void TestXmlItemSerializer::testEnclosures()
{
    RssItem item;
    item.setLink( QLatin1String("http://akregator.kde.org") );
    Enclosure enc;
    enc.setUrl( QLatin1String("http://akregator.kde.org/some.mp3") );
    enc.setType( QLatin1String("audio/mpeg") );
    enc.setTitle( QLatin1String("This is an enclosure!") );
    enc.setLength( 123456789 );
    Enclosure enc2 = enc;
    enc.setDuration( 60 );
    QList<Enclosure> encs;
    encs.append( enc );
    encs.append( enc2 );
    item.setEnclosures( encs );
    ::testItem( item );
}

void TestXmlItemSerializer::testCategories()
{
    Category cat;
    cat.setTerm( QLatin1String("term1") );
    cat.setScheme( QLatin1String("http://Blabla#") );
    cat.setLabel( QLatin1String("Term 1") );
    Category cat2;
    cat2.setTerm( QLatin1String("Term2") );
    cat2.setLabel( QLatin1String("Tada") );
    QList<Category> cats;
    cats.append( cat );
    cats.append( cat2 );
    cats.append( Category() );
    RssItem item;
    item.setCategories( cats );
    ::testItem( item );
}

void TestXmlItemSerializer::testAuthors()
{
    Person a1;
    a1.setName( QLatin1String("John Doe") );
    Person a2;
    a2.setName( QLatin1String("John Doe") );
    a2.setUri( QLatin1String("http://doeweb.net/John") );
    a2.setEmail( QLatin1String("joe@doeweb.net") );
    Person a3;
    QList<Person> authors;
    authors.append( a1 );
    authors.append( a2 );
    authors.append( a3 );
    RssItem item;
    item.setAuthors( authors );
    ::testItem( item );
}

void TestXmlItemSerializer::testComments()
{
    RssItem item;
    item.setCommentsCount( 10 );
    item.setCommentsLink( QLatin1String("http://heyho#comment") );
    item.setCommentsFeed( QLatin1String("http://heyho/comments.rss") );
    item.setCommentPostUri( QLatin1String("http://whatever") );
    ::testItem( item );
}


QTEST_KDEMAIN( TestXmlItemSerializer, NoGUI )

#include "testxmlitemserializer.moc"
