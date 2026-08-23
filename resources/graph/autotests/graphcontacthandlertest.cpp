/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Unit tests for the Graph contact JSON <-> KContacts::Addressee mapping.
*/

#include "contact/graphcontacthandler.h"

#include <KContacts/Address>
#include <KContacts/PhoneNumber>

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

using namespace KContacts;

namespace
{
QJsonObject sampleContact()
{
    QJsonObject json;
    json.insert(QStringLiteral("givenName"), QStringLiteral("Jürgen"));
    json.insert(QStringLiteral("surname"), QStringLiteral("Müßig"));
    json.insert(QStringLiteral("middleName"), QStringLiteral("K."));
    json.insert(QStringLiteral("title"), QStringLiteral("Dr."));
    json.insert(QStringLiteral("nickName"), QStringLiteral("Jü"));
    json.insert(QStringLiteral("displayName"), QStringLiteral("Dr. Jürgen Müßig"));
    json.insert(QStringLiteral("jobTitle"), QStringLiteral("Engineer"));
    json.insert(QStringLiteral("companyName"), QStringLiteral("Example GmbH"));
    json.insert(QStringLiteral("department"), QStringLiteral("R&D"));
    json.insert(QStringLiteral("personalNotes"), QStringLiteral("Prefers e-mail"));

    QJsonArray emails;
    QJsonObject email1;
    email1.insert(QStringLiteral("address"), QStringLiteral("juergen@example.com"));
    QJsonObject email2;
    email2.insert(QStringLiteral("address"), QStringLiteral("j.muessig@example.org"));
    emails.append(email1);
    emails.append(email2);
    json.insert(QStringLiteral("emailAddresses"), emails);

    json.insert(QStringLiteral("businessPhones"), QJsonArray{QStringLiteral("+49 30 123456")});
    json.insert(QStringLiteral("homePhones"), QJsonArray{QStringLiteral("+49 30 654321")});
    json.insert(QStringLiteral("mobilePhone"), QStringLiteral("+49 170 1234567"));

    QJsonObject home;
    home.insert(QStringLiteral("street"), QStringLiteral("Musterstraße 1"));
    home.insert(QStringLiteral("city"), QStringLiteral("Berlin"));
    home.insert(QStringLiteral("state"), QStringLiteral("Berlin"));
    home.insert(QStringLiteral("postalCode"), QStringLiteral("10115"));
    home.insert(QStringLiteral("countryOrRegion"), QStringLiteral("Germany"));
    json.insert(QStringLiteral("homeAddress"), home);

    json.insert(QStringLiteral("birthday"), QStringLiteral("1980-05-15T00:00:00Z"));
    json.insert(QStringLiteral("categories"), QJsonArray{QStringLiteral("Familie"), QStringLiteral("Verein")});
    return json;
}
} // namespace

class GraphContactHandlerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shouldMapNamesAndOrganization()
    {
        const Addressee a = GraphContactHandler::toAddressee(sampleContact());
        QCOMPARE(a.givenName(), QStringLiteral("Jürgen"));
        QCOMPARE(a.familyName(), QStringLiteral("Müßig"));
        QCOMPARE(a.additionalName(), QStringLiteral("K."));
        QCOMPARE(a.prefix(), QStringLiteral("Dr."));
        QCOMPARE(a.nickName(), QStringLiteral("Jü"));
        QCOMPARE(a.formattedName(), QStringLiteral("Dr. Jürgen Müßig"));
        QCOMPARE(a.title(), QStringLiteral("Engineer"));
        QCOMPARE(a.organization(), QStringLiteral("Example GmbH"));
        QCOMPARE(a.department(), QStringLiteral("R&D"));
        QCOMPARE(a.note(), QStringLiteral("Prefers e-mail"));
    }

    void shouldMapEmailsWithFirstPreferred()
    {
        const Addressee a = GraphContactHandler::toAddressee(sampleContact());
        const auto emails = a.emailList();
        QCOMPARE(emails.size(), 2);
        QCOMPARE(emails.at(0).mail(), QStringLiteral("juergen@example.com"));
        QVERIFY(emails.at(0).isPreferred());
        QCOMPARE(emails.at(1).mail(), QStringLiteral("j.muessig@example.org"));
        QVERIFY(!emails.at(1).isPreferred());
    }

    void shouldMapPhones()
    {
        const Addressee a = GraphContactHandlerTest::addressee();
        QCOMPARE(a.phoneNumbers().size(), 3);
        QCOMPARE(a.phoneNumber(PhoneNumber::Work).number(), QStringLiteral("+49 30 123456"));
        QCOMPARE(a.phoneNumber(PhoneNumber::Home).number(), QStringLiteral("+49 30 654321"));
        QCOMPARE(a.phoneNumber(PhoneNumber::Cell).number(), QStringLiteral("+49 170 1234567"));
    }

    void shouldMapAddressAndBirthday()
    {
        const Addressee a = GraphContactHandlerTest::addressee();
        const Address home = a.address(Address::Home);
        QCOMPARE(home.street(), QStringLiteral("Musterstraße 1"));
        QCOMPARE(home.locality(), QStringLiteral("Berlin"));
        QCOMPARE(home.postalCode(), QStringLiteral("10115"));
        QCOMPARE(home.country(), QStringLiteral("Germany"));
        QCOMPARE(a.birthday().date(), QDate(1980, 5, 15));
    }

    void shouldMapCategories()
    {
        const Addressee a = GraphContactHandlerTest::addressee();
        QCOMPARE(a.categories(), QStringList({QStringLiteral("Familie"), QStringLiteral("Verein")}));
    }

    void shouldRoundTripThroughJson()
    {
        const QJsonObject json = sampleContact();
        const QJsonObject back = GraphContactHandler::toJson(GraphContactHandler::toAddressee(json));

        QCOMPARE(back.value(QLatin1String("givenName")), json.value(QLatin1String("givenName")));
        QCOMPARE(back.value(QLatin1String("surname")), json.value(QLatin1String("surname")));
        QCOMPARE(back.value(QLatin1String("displayName")), json.value(QLatin1String("displayName")));
        QCOMPARE(back.value(QLatin1String("companyName")), json.value(QLatin1String("companyName")));
        QCOMPARE(back.value(QLatin1String("emailAddresses")), json.value(QLatin1String("emailAddresses")));
        QCOMPARE(back.value(QLatin1String("businessPhones")), json.value(QLatin1String("businessPhones")));
        QCOMPARE(back.value(QLatin1String("homePhones")), json.value(QLatin1String("homePhones")));
        QCOMPARE(back.value(QLatin1String("mobilePhone")), json.value(QLatin1String("mobilePhone")));
        QCOMPARE(back.value(QLatin1String("homeAddress")), json.value(QLatin1String("homeAddress")));
        QCOMPARE(back.value(QLatin1String("categories")), json.value(QLatin1String("categories")));
    }

    void shouldUseVCardMimeType()
    {
        QCOMPARE(GraphContactHandler::mimeType(), Addressee::mimeType());
    }

private:
    static Addressee addressee()
    {
        return GraphContactHandler::toAddressee(sampleContact());
    }
};

QTEST_GUILESS_MAIN(GraphContactHandlerTest)

#include "graphcontacthandlertest.moc"
