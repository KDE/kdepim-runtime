/*
    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtCore/QObject>
#include <akonadi/qtest_akonadi.h>
#include <KDebug>
#include <Soprano/Vocabulary/NAO>
#include <nco/contact.h>
#include <nco/emailaddress.h>
#include <nmo/email.h>

#include <propertycache.h>

class CacheTest: public QObject
{
    Q_OBJECT
private slots:
    void testCache()
    {
        PropertyCache cache;
        cache.setCachedTypes(QList<QUrl>()
           << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#EmailAddress")
           << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact")
        );
        
        Nepomuk2::SimpleResourceGraph graph;
        Nepomuk2::SimpleResource res;
        Nepomuk2::NMO::Email mail( &res );
        QList<QUrl> contacts;
        Nepomuk2::SimpleResource contactRes;
        Nepomuk2::NCO::Contact contact( &contactRes );
        contactRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), "name" );
        Nepomuk2::SimpleResource emailRes;
        Nepomuk2::NCO::EmailAddress email( &emailRes );
        email.setEmailAddress( "test@example.org" );
        graph << emailRes;
        contact.addHasEmailAddress( emailRes.uri() );
        graph << contactRes;
        contacts << contactRes.uri();
        mail.setTos( contacts );
        graph << res;
        
        QHash<QUrl, QUrl> mapping;
        mapping.insert(res.uri(), QUrl("nepomuk://a"));
        mapping.insert(contactRes.uri(), QUrl("nepomuk://b"));
        mapping.insert(emailRes.uri(), QUrl("nepomuk://c"));
        cache.fillCache(graph, mapping);
//         kDebug() << graph;

        Nepomuk2::SimpleResourceGraph result = cache.applyCache(graph);
        kDebug() << result;
        QCOMPARE(result.toList().size(), 1);
        Nepomuk2::SimpleResource res1 = result.toList().at(0);
        QCOMPARE(res1.uri(), res.uri());
        QCOMPARE(res1.property(QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to")).first().toUrl(), QUrl("nepomuk://b"));
    }
    
    void testCacheLimit()
    {
        PropertyCache cache;
        cache.setCachedTypes(QList<QUrl>()
           << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#EmailAddress")
           << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact")
        );
        cache.setSize(1);
        
        Nepomuk2::SimpleResourceGraph graph;
        Nepomuk2::SimpleResource res;
        Nepomuk2::NMO::Email mail( &res );
        QList<QUrl> contacts;
        Nepomuk2::SimpleResource contactRes;
        Nepomuk2::NCO::Contact contact( &contactRes );
        contactRes.setProperty( Soprano::Vocabulary::NAO::prefLabel(), "name" );
        Nepomuk2::SimpleResource emailRes;
        Nepomuk2::NCO::EmailAddress email( &emailRes );
        email.setEmailAddress( "test@example.org" );
        graph << emailRes;
        contact.addHasEmailAddress( emailRes.uri() );
        graph << contactRes;
        contacts << contactRes.uri();
        mail.setTos( contacts );
        graph << res;
        
        QHash<QUrl, QUrl> mapping;
        mapping.insert(res.uri(), QUrl("nepomuk://a"));
        mapping.insert(contactRes.uri(), QUrl("nepomuk://b"));
        mapping.insert(emailRes.uri(), QUrl("nepomuk://c"));
        cache.fillCache(graph, mapping);
//         kDebug() << graph;

        Nepomuk2::SimpleResourceGraph result = cache.applyCache(graph);
        kDebug() << result;
        QCOMPARE(result.toList().size(), 2);
        Nepomuk2::SimpleResource res1 = result.toList().at(0);
        QCOMPARE(res1.uri(), res.uri());
        QCOMPARE(res1.property(QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to")).first().toUrl(), QUrl("nepomuk://b"));
        Nepomuk2::SimpleResource res2 = result.toList().at(1);
        QCOMPARE(res2.uri(), emailRes.uri());
        QCOMPARE(res2.property(QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress")).first().toString(), QString("test@example.org"));
    }


};

QTEST_AKONADIMAIN( CacheTest, NoGUI )

#include "cachetest.moc"