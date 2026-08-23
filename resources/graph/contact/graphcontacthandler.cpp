/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphcontacthandler.h"

#include <KContacts/Address>
#include <KContacts/Email>
#include <KContacts/PhoneNumber>

#include <QDateTime>
#include <QJsonArray>

using namespace KContacts;

namespace
{
void addPhones(Addressee &a, const QJsonArray &numbers, PhoneNumber::Type type)
{
    for (const auto &n : numbers) {
        const QString num = n.toString();
        if (!num.isEmpty()) {
            a.insertPhoneNumber(PhoneNumber(num, type));
        }
    }
}

Address toAddress(const QJsonObject &json, Address::Type type)
{
    Address addr(type);
    addr.setStreet(json.value(QLatin1String("street")).toString());
    addr.setLocality(json.value(QLatin1String("city")).toString());
    addr.setRegion(json.value(QLatin1String("state")).toString());
    addr.setPostalCode(json.value(QLatin1String("postalCode")).toString());
    addr.setCountry(json.value(QLatin1String("countryOrRegion")).toString());
    return addr;
}

QJsonObject addressToJson(const Address &addr)
{
    QJsonObject o;
    o.insert(QStringLiteral("street"), addr.street());
    o.insert(QStringLiteral("city"), addr.locality());
    o.insert(QStringLiteral("state"), addr.region());
    o.insert(QStringLiteral("postalCode"), addr.postalCode());
    o.insert(QStringLiteral("countryOrRegion"), addr.country());
    return o;
}
} // namespace

namespace GraphContactHandler
{
QString mimeType()
{
    return KContacts::Addressee::mimeType();
}

KContacts::Addressee toAddressee(const QJsonObject &json)
{
    Addressee a;
    a.setGivenName(json.value(QLatin1String("givenName")).toString());
    a.setFamilyName(json.value(QLatin1String("surname")).toString());
    a.setAdditionalName(json.value(QLatin1String("middleName")).toString());
    a.setPrefix(json.value(QLatin1String("title")).toString());
    a.setNickName(json.value(QLatin1String("nickName")).toString());
    a.setFormattedName(json.value(QLatin1String("displayName")).toString());

    const QJsonArray emails = json.value(QLatin1String("emailAddresses")).toArray();
    for (int i = 0; i < emails.size(); ++i) {
        const QString addr = emails.at(i).toObject().value(QLatin1String("address")).toString();
        if (!addr.isEmpty()) {
            Email email(addr);
            email.setPreferred(i == 0);
            a.addEmail(email);
        }
    }

    addPhones(a, json.value(QLatin1String("businessPhones")).toArray(), PhoneNumber::Work);
    addPhones(a, json.value(QLatin1String("homePhones")).toArray(), PhoneNumber::Home);
    const QString mobile = json.value(QLatin1String("mobilePhone")).toString();
    if (!mobile.isEmpty()) {
        a.insertPhoneNumber(PhoneNumber(mobile, PhoneNumber::Cell));
    }

    a.setTitle(json.value(QLatin1String("jobTitle")).toString());
    a.setOrganization(json.value(QLatin1String("companyName")).toString());
    a.setDepartment(json.value(QLatin1String("department")).toString());
    a.setNote(json.value(QLatin1String("personalNotes")).toString());

    const QJsonObject business = json.value(QLatin1String("businessAddress")).toObject();
    if (!business.isEmpty() && business.contains(QLatin1String("street"))) {
        a.insertAddress(toAddress(business, Address::Work));
    }
    const QJsonObject home = json.value(QLatin1String("homeAddress")).toObject();
    if (!home.isEmpty() && home.contains(QLatin1String("street"))) {
        a.insertAddress(toAddress(home, Address::Home));
    }

    const QString birthday = json.value(QLatin1String("birthday")).toString();
    if (!birthday.isEmpty()) {
        const QDateTime bd = QDateTime::fromString(birthday, Qt::ISODate);
        if (bd.isValid()) {
            a.setBirthday(bd.date());
        }
    }

    QStringList categories;
    const QJsonArray cats = json.value(QLatin1String("categories")).toArray();
    categories.reserve(cats.count());
    for (const auto &c : cats) {
        const QString category = c.toString();
        if (!category.isEmpty()) {
            categories << category;
        }
    }
    a.setCategories(categories);

    // The photo is not part of the contact resource; GraphFetchPimItemsJob fetches
    // it separately from /me/contacts/{id}/photo/$value and sets it on the Addressee.
    return a;
}

QJsonObject toJson(const KContacts::Addressee &a)
{
    QJsonObject json;
    json.insert(QStringLiteral("givenName"), a.givenName());
    json.insert(QStringLiteral("surname"), a.familyName());
    json.insert(QStringLiteral("middleName"), a.additionalName());
    json.insert(QStringLiteral("title"), a.prefix());
    json.insert(QStringLiteral("nickName"), a.nickName());
    json.insert(QStringLiteral("displayName"), a.formattedName());
    json.insert(QStringLiteral("jobTitle"), a.title());
    json.insert(QStringLiteral("companyName"), a.organization());
    json.insert(QStringLiteral("department"), a.department());
    json.insert(QStringLiteral("personalNotes"), a.note());
    // birthday is read in toAddressee() but deliberately never written back:
    // Exchange reacts to a birthday write by creating a recurring event in the
    // Birthdays calendar, which a plain contact edit must not silently do.

    QJsonArray emails;
    const auto emailList = a.emails();
    for (const QString &e : emailList) {
        QJsonObject eo;
        eo.insert(QStringLiteral("address"), e);
        emails.append(eo);
    }
    json.insert(QStringLiteral("emailAddresses"), emails);

    QJsonArray business, home;
    const auto phones = a.phoneNumbers();
    QString mobile;
    for (const PhoneNumber &p : phones) {
        if (p.type() & PhoneNumber::Cell) {
            mobile = p.number();
        } else if (p.type() & PhoneNumber::Home) {
            home.append(p.number());
        } else {
            business.append(p.number());
        }
    }
    json.insert(QStringLiteral("businessPhones"), business);
    json.insert(QStringLiteral("homePhones"), home);
    if (!mobile.isEmpty()) {
        json.insert(QStringLiteral("mobilePhone"), mobile);
    }

    const Address work = a.address(Address::Work);
    if (!work.isEmpty()) {
        json.insert(QStringLiteral("businessAddress"), addressToJson(work));
    }
    const Address homeAddr = a.address(Address::Home);
    if (!homeAddr.isEmpty()) {
        json.insert(QStringLiteral("homeAddress"), addressToJson(homeAddr));
    }

    json.insert(QStringLiteral("categories"), QJsonArray::fromStringList(a.categories()));
    return json;
}
}
