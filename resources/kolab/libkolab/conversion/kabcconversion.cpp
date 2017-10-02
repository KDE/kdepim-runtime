/*
 * Copyright (C) 2011  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kabcconversion.h"

#include "commonconversion.h"
#include <qbuffer.h>
#include <qimagereader.h>
#include "kolabformat/errorhandler.h"

namespace Kolab {
namespace Conversion {
//The following was copied from kdepim/libkleo/kleo/enum.h,.cpp
enum CryptoMessageFormat {
    InlineOpenPGPFormat = 1,
    OpenPGPMIMEFormat = 2,
    SMIMEFormat = 4,
    SMIMEOpaqueFormat = 8,
    AnyOpenPGP = InlineOpenPGPFormat|OpenPGPMIMEFormat,
    AnySMIME = SMIMEOpaqueFormat|SMIMEFormat,
    AutoFormat = AnyOpenPGP|AnySMIME
};

enum EncryptionPreference {
    UnknownPreference = 0,
    NeverEncrypt = 1,
    AlwaysEncrypt = 2,
    AlwaysEncryptIfPossible = 3,
    AlwaysAskForEncryption = 4,
    AskWheneverPossible = 5,
    MaxEncryptionPreference = AskWheneverPossible
};

enum SigningPreference {
    UnknownSigningPreference = 0,
    NeverSign = 1,
    AlwaysSign = 2,
    AlwaysSignIfPossible = 3,
    AlwaysAskForSigning = 4,
    AskSigningWheneverPossible = 5,
    MaxSigningPreference = AskSigningWheneverPossible
};

static const struct {
    CryptoMessageFormat format;
    const char *displayName;
    const char *configName;
} cryptoMessageFormats[] = {
    { InlineOpenPGPFormat,
      ("Inline OpenPGP (deprecated)"),
      "inline openpgp" },
    { OpenPGPMIMEFormat,
      ("OpenPGP/MIME"),
      "openpgp/mime" },
    { SMIMEFormat,
      ("S/MIME"),
      "s/mime" },
    { SMIMEOpaqueFormat,
      ("S/MIME Opaque"),
      "s/mime opaque" },
};
static const unsigned int numCryptoMessageFormats
    = sizeof cryptoMessageFormats / sizeof *cryptoMessageFormats;

const char *cryptoMessageFormatToString(CryptoMessageFormat f)
{
    if (f == AutoFormat) {
        return "auto";
    }
    for (unsigned int i = 0; i < numCryptoMessageFormats; ++i) {
        if (f == cryptoMessageFormats[i].format) {
            return cryptoMessageFormats[i].configName;
        }
    }
    return nullptr;
}

QStringList cryptoMessageFormatsToStringList(unsigned int f)
{
    QStringList result;
    for (unsigned int i = 0; i < numCryptoMessageFormats; ++i) {
        if (f & cryptoMessageFormats[i].format) {
            result.push_back(cryptoMessageFormats[i].configName);
        }
    }
    return result;
}

CryptoMessageFormat stringToCryptoMessageFormat(const QString &s)
{
    const QString t = s.toLower();
    for (unsigned int i = 0; i < numCryptoMessageFormats; ++i) {
        if (t == cryptoMessageFormats[i].configName) {
            return cryptoMessageFormats[i].format;
        }
    }
    return AutoFormat;
}

unsigned int stringListToCryptoMessageFormats(const QStringList &sl)
{
    unsigned int result = 0;
    const QStringList::const_iterator end(sl.end());
    for (QStringList::const_iterator it = sl.begin(); it != end; ++it) {
        result |= stringToCryptoMessageFormat(*it);
    }
    return result;
}

// For the config values used below, see also kaddressbook/editors/cryptowidget.cpp

const char *encryptionPreferenceToString(EncryptionPreference pref)
{
    switch (pref) {
    case UnknownPreference:
        return nullptr;
    case NeverEncrypt:
        return "never";
    case AlwaysEncrypt:
        return "always";
    case AlwaysEncryptIfPossible:
        return "alwaysIfPossible";
    case AlwaysAskForEncryption:
        return "askAlways";
    case AskWheneverPossible:
        return "askWhenPossible";
    }
    return nullptr;         // keep the compiler happy
}

EncryptionPreference stringToEncryptionPreference(const QString &str)
{
    if (str == QLatin1String("never")) {
        return NeverEncrypt;
    }
    if (str == QLatin1String("always")) {
        return AlwaysEncrypt;
    }
    if (str == QLatin1String("alwaysIfPossible")) {
        return AlwaysEncryptIfPossible;
    }
    if (str == QLatin1String("askAlways")) {
        return AlwaysAskForEncryption;
    }
    if (str == QLatin1String("askWhenPossible")) {
        return AskWheneverPossible;
    }
    return UnknownPreference;
}

const char *signingPreferenceToString(SigningPreference pref)
{
    switch (pref) {
    case UnknownSigningPreference:
        return nullptr;
    case NeverSign:
        return "never";
    case AlwaysSign:
        return "always";
    case AlwaysSignIfPossible:
        return "alwaysIfPossible";
    case AlwaysAskForSigning:
        return "askAlways";
    case AskSigningWheneverPossible:
        return "askWhenPossible";
    }
    return nullptr;         // keep the compiler happy
}

SigningPreference stringToSigningPreference(const QString &str)
{
    if (str == QStringLiteral("never")) {
        return NeverSign;
    }
    if (str == QStringLiteral("always")) {
        return AlwaysSign;
    }
    if (str == QStringLiteral("alwaysIfPossible")) {
        return AlwaysSignIfPossible;
    }
    if (str == QStringLiteral("askAlways")) {
        return AlwaysAskForSigning;
    }
    if (str == QStringLiteral("askWhenPossible")) {
        return AskSigningWheneverPossible;
    }
    return UnknownSigningPreference;
}

int fromAddressType(int kabcType, bool &pref)
{
    int type = 0;
    if (kabcType & KContacts::Address::Dom) {
        Warning() << "domestic address is not supported";
    }
    if (kabcType & KContacts::Address::Intl) {
        Warning() << "international address is not supported";
    }
    if (kabcType & KContacts::Address::Pref) {
        pref = true;
    }
    if (kabcType & KContacts::Address::Postal) {
        Warning() << "postal address is not supported";
    }
    if (kabcType & KContacts::Address::Parcel) {
        Warning() << "parcel is not supported";
    }
    if (kabcType & KContacts::Address::Home) {
        type |= Kolab::Address::Home;
    }
    if (kabcType & KContacts::Address::Work) {
        type |= Kolab::Address::Work;
    }
    return type;
}

KContacts::Address::Type toAddressType(int types, bool pref)
{
    KContacts::Address::Type type = 0;
    if (pref) {
        type |= KContacts::Address::Pref;
    }
    if (types & Kolab::Address::Home) {
        type |= KContacts::Address::Home;
    }
    if (types & Kolab::Address::Work) {
        type |= KContacts::Address::Work;
    }
    return type;
}

int fromPhoneType(int kabcType, bool &pref)
{
    int type = 0;
    if (kabcType & KContacts::PhoneNumber::Home) {
        type |= Kolab::Telephone::Home;
    }
    if (kabcType & KContacts::PhoneNumber::Work) {
        type |= Kolab::Telephone::Work;
    }
    if (kabcType & KContacts::PhoneNumber::Msg) {
        type |= Kolab::Telephone::Text;
    }
    if (kabcType & KContacts::PhoneNumber::Pref) {
        pref = true;
    }
    if (kabcType & KContacts::PhoneNumber::Voice) {
        type |= Kolab::Telephone::Voice;
    }
    if (kabcType & KContacts::PhoneNumber::Fax) {
        type |= Kolab::Telephone::Fax;
    }
    if (kabcType & KContacts::PhoneNumber::Cell) {
        type |= Kolab::Telephone::Cell;
    }
    if (kabcType & KContacts::PhoneNumber::Video) {
        type |= Kolab::Telephone::Video;
    }
    if (kabcType & KContacts::PhoneNumber::Bbs) {
        Warning() << "mailbox number is not supported";
    }
    if (kabcType & KContacts::PhoneNumber::Modem) {
        Warning() << "modem is not supported";
    }
    if (kabcType & KContacts::PhoneNumber::Car) {
        type |= Kolab::Telephone::Car;
    }
    if (kabcType & KContacts::PhoneNumber::Isdn) {
        Warning() << "isdn number is not supported";
    }
    if (kabcType & KContacts::PhoneNumber::Pcs) {
        type |= Kolab::Telephone::Text;
    }
    if (kabcType & KContacts::PhoneNumber::Pager) {
        type |= Kolab::Telephone::Pager;
    }
    return type;
}

KContacts::PhoneNumber::Type toPhoneType(int types, bool pref)
{
    KContacts::PhoneNumber::Type type = 0;
    if (types & Kolab::Telephone::Home) {
        type |= KContacts::PhoneNumber::Home;
    }
    if (types & Kolab::Telephone::Work) {
        type |= KContacts::PhoneNumber::Work;
    }
    if (types & Kolab::Telephone::Text) {
        type |= KContacts::PhoneNumber::Msg;
    }
    if (pref) {
        type |= KContacts::PhoneNumber::Pref;
    }
    if (types & Kolab::Telephone::Voice) {
        type |= KContacts::PhoneNumber::Voice;
    }
    if (types & Kolab::Telephone::Fax) {
        type |= KContacts::PhoneNumber::Fax;
    }
    if (types & Kolab::Telephone::Cell) {
        type |= KContacts::PhoneNumber::Cell;
    }
    if (types & Kolab::Telephone::Video) {
        type |= KContacts::PhoneNumber::Video;
    }
    if (types & Kolab::Telephone::Car) {
        type |= KContacts::PhoneNumber::Car;
    }
    if (types & Kolab::Telephone::Text) {
        type |= KContacts::PhoneNumber::Pcs;
    }
    if (types & Kolab::Telephone::Pager) {
        type |= KContacts::PhoneNumber::Pager;
    }
    return type;
}

std::string fromPicture(const KContacts::Picture &pic, std::string &mimetype)
{
    QByteArray input;
    QBuffer buffer(&input);
    buffer.open(QIODevice::WriteOnly);
    QImage img;

    if (pic.isIntern()) {
        if (!pic.data().isNull()) {
            img = pic.data();
        }
    } else if (!pic.url().isEmpty()) {
        QString tmpFile;
        qWarning() << "external pictures are currently not supported";
        //FIXME add kio support to libcalendaring or use libcurl
//         if ( KIO::NetAccess::download( pic.url(), tmpFile, 0 /*no widget known*/ ) ) {
//             img.load( tmpFile );
//             KIO::NetAccess::removeTempFile( tmpFile );
//         }
    }
    if (img.isNull()) {
        Error() << "invalid picture";
        return std::string();
    }
    if (!img.hasAlphaChannel()) {
        if (!img.save(&buffer, "JPEG")) {
            Error() << "error on jpeg save";
            return std::string();
        }
        mimetype = "image/jpeg";
    } else {
        if (!img.save(&buffer, "PNG")) {
            Error() << "error on png save";
            return std::string();
        }
        mimetype = "image/png";
    }
    return std::string(input.data(), input.size());
}

KContacts::Picture toPicture(const std::string &data, const std::string &mimetype)
{
    QImage img;
    bool ret = false;
    QByteArray type(mimetype.data(), mimetype.size());
    type = type.split('/').last(); // extract "jpeg" from "image/jpeg"
    if (QImageReader::supportedImageFormats().contains(type)) {
        ret = img.loadFromData(QByteArray::fromRawData(data.data(), data.size()), type.constData());
    } else {
        ret = img.loadFromData(QByteArray::fromRawData(data.data(), data.size()));
    }
    if (!ret) {
        Warning() << "failed to load picture";
        return KContacts::Picture();
    }

    KContacts::Picture logo(img);
    if (logo.isEmpty()) {
        Warning() << "failed to read picture";
        return KContacts::Picture();
    }
    return logo;
}

template<typename T>
void setCustom(const std::string &value, const std::string &id, T &object)
{
    std::vector <Kolab::CustomProperty > properties = object.customProperties();
    properties.push_back(CustomProperty(id, value));
    object.setCustomProperties(properties);
}

template<typename T>
std::string getCustom(const std::string &id, T &object)
{
    const std::vector <Kolab::CustomProperty > &properties = object.customProperties();
    foreach (const Kolab::CustomProperty &prop, properties) {
        if (prop.identifier == id) {
            return prop.value;
        }
    }
    return std::string();
}

static QString emailTypesToStringList(int emailTypes)
{
    QStringList types;
    if (emailTypes & Kolab::Email::Home) {
        types << QStringLiteral("home");
    }
    if (emailTypes & Kolab::Email::Work) {
        types << QStringLiteral("work");
    }
    return types.join(QLatin1Char(','));
}

static int emailTypesFromStringlist(const QString &types)
{
    int emailTypes = Kolab::Email::NoType;
    if (types.contains(QStringLiteral("home"))) {
        emailTypes |= Kolab::Email::Home;
    }
    if (types.contains(QStringLiteral("work"))) {
        emailTypes |= Kolab::Email::Work;
    }
    return emailTypes;
}

KContacts::Addressee toKABC(const Kolab::Contact &contact)
{
    KContacts::Addressee addressee;
    addressee.setUid(fromStdString(contact.uid()));
    addressee.setCategories(toStringList(contact.categories()));
    //addressee.setName(fromStdString(contact.name()));//This one is only for compatiblity (and results in a non-existing name property)
    addressee.setFormattedName(fromStdString(contact.name())); //This on corresponds to fn

    const Kolab::NameComponents &nc = contact.nameComponents();
    if (!nc.surnames().empty()) {
        addressee.setFamilyName(fromStdString(nc.surnames().front()));
    }
    if (!nc.given().empty()) {
        addressee.setGivenName(fromStdString(nc.given().front()));
    }
    if (!nc.additional().empty()) {
        addressee.setAdditionalName(fromStdString(nc.additional().front()));
    }
    if (!nc.prefixes().empty()) {
        addressee.setPrefix(fromStdString(nc.prefixes().front()));
    }
    if (!nc.suffixes().empty()) {
        addressee.setSuffix(fromStdString(nc.suffixes().front()));
    }

    addressee.setNote(fromStdString(contact.note()));

    addressee.setSecrecy(KContacts::Secrecy::Public); //We don't have any privacy setting in xCard

    QString preferredEmail;

    if (!contact.emailAddresses().empty()) {
        QStringList emails;
        foreach (const Kolab::Email &email, contact.emailAddresses()) {
            emails << fromStdString(email.address());
            const QString types = emailTypesToStringList(email.types());
            if (!types.isEmpty()) {
                addressee.insertCustom(QLatin1String("KOLAB"), QString::fromLatin1("EmailTypes%1").arg(fromStdString(email.address())), types);
            }
        }
        addressee.setEmails(emails);
        if ((contact.emailAddressPreferredIndex() >= 0) && (contact.emailAddressPreferredIndex() < static_cast<int>(contact.emailAddresses().size()))) {
            preferredEmail = fromStdString(contact.emailAddresses().at(contact.emailAddressPreferredIndex()).address());
        } else {
            preferredEmail = fromStdString(contact.emailAddresses().at(0).address());
        }
        addressee.insertEmail(preferredEmail, true);
    }

    if (!contact.freeBusyUrl().empty()) {
        if (preferredEmail.isEmpty()) {
            Error() << "f/b url is set but no email address available, skipping";
        } else {
            addressee.insertCustom(QStringLiteral("KOLAB"), QStringLiteral("FreebusyUrl"), fromStdString(contact.freeBusyUrl()));
        }
    }

    if (!contact.nickNames().empty()) {
        addressee.setNickName(fromStdString(contact.nickNames().at(0))); //TODO support multiple
    }

    if (contact.bDay().isValid()) {
        addressee.setBirthday(toDate(contact.bDay()));
    }
    if (!contact.titles().empty()) {
        addressee.setTitle(fromStdString(contact.titles().at(0))); //TODO support multiple
    }
    if (!contact.urls().empty()) {
        KContacts::ResourceLocatorUrl url;
        url.setUrl(QUrl(fromStdString(contact.urls().at(0).url()))); //TODO support multiple
        addressee.setUrl(url);
        foreach (const Kolab::Url &u, contact.urls()) {
            if (u.type() == Kolab::Url::Blog) {
                addressee.insertCustom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("BlogFeed"), fromStdString(u.url()));
            }
        }
    }

    if (!contact.affiliations().empty()) {
        //Storing only a const reference leads to segfaults. No idea why.
        const Kolab::Affiliation aff = contact.affiliations().at(0); //TODO support multiple
        if (!aff.organisation().empty()) {
            addressee.setOrganization(fromStdString(aff.organisation()));
        }
        if (!aff.organisationalUnits().empty()) {
            addressee.setDepartment(fromStdString(aff.organisationalUnits().at(0))); //TODO support multiple
        }
        if (!aff.roles().empty()) {
            addressee.setRole(fromStdString(aff.roles().at(0))); //TODO support multiple
        }
        if (!aff.logo().empty()) {
            addressee.setLogo(toPicture(aff.logo(), aff.logoMimetype()));
        }
        foreach (const Kolab::Related &related, aff.relateds()) {
            if (related.type() != Kolab::Related::Text) {
                Error() << "invalid relation type";
                continue;
            }
            if (related.relationTypes() & Kolab::Related::Assistant) {
                addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-AssistantsName"), fromStdString(related.text()));
            }
            if (related.relationTypes() & Kolab::Related::Manager) {
                addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-ManagersName"), fromStdString(related.text()));
            }
        }
        foreach (const Kolab::Address &address, aff.addresses()) {
            addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-Office"), fromStdString(address.label())); //TODO support proper addresses
        }
    }
    const std::string &prof = getCustom("X-Profession", contact);
    if (!prof.empty()) {
        addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-Profession"), fromStdString(prof));
    }

    const std::string &adrBook = getCustom("X-AddressBook", contact);
    if (!adrBook.empty()) {
        addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-AddressBook"), fromStdString(prof));
    }

    if (!contact.photo().empty()) {
        addressee.setPhoto(toPicture(contact.photo(), contact.photoMimetype()));
    }

    if (!contact.telephones().empty()) {
        int index = 0;
        foreach (const Kolab::Telephone &tel, contact.telephones()) {
            bool pref = false;
            if (index == contact.telephonesPreferredIndex()) {
                pref = true;
            }
            KContacts::PhoneNumber number(fromStdString(tel.number()), toPhoneType(tel.types(), pref));
            index++;
            addressee.insertPhoneNumber(number);
        }
    }

    if (!contact.addresses().empty()) {
        int index = 0;
        foreach (const Kolab::Address &a, contact.addresses()) {
            bool pref = false;
            if (index == contact.addressPreferredIndex()) {
                pref = true;
            }
            KContacts::Address adr(toAddressType(a.types(), pref));
            adr.setLabel(fromStdString(a.label()));
            adr.setStreet(fromStdString(a.street()));
            adr.setLocality(fromStdString(a.locality()));
            adr.setRegion(fromStdString(a.region()));
            adr.setPostalCode(fromStdString(a.code()));
            adr.setCountry(fromStdString(a.country()));

            index++;
            addressee.insertAddress(adr);
        }
    }

    if (contact.anniversary().isValid()) {
        addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-Anniversary"), toDate(contact.anniversary()).toString(Qt::ISODate));
    }

    if (!contact.imAddresses().empty()) {
        addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-IMAddress"), fromStdString(contact.imAddresses()[0])); //TODO support multiple
    }

    if (!contact.relateds().empty()) {
        foreach (const Kolab::Related &rel, contact.relateds()) {
            if (rel.type() != Kolab::Related::Text) {
                Error() << "relation type not supported";
                continue;
            }
            if (rel.relationTypes() & Kolab::Related::Spouse) {
                addressee.insertCustom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-SpousesName"), fromStdString(rel.text())); //TODO support multiple
            } else {
                Warning() << "relation not supported";
                continue;
            }
        }
    }

    return addressee;
}

Kolab::Contact fromKABC(const KContacts::Addressee &addressee)
{
    int prefNum = -1;
    int prefCounter = -1;
    Kolab::Contact c;
    c.setUid(toStdString(addressee.uid()));
    c.setCategories(fromStringList(addressee.categories()));
    c.setName(toStdString(addressee.formattedName()));
    Kolab::NameComponents nc;
    nc.setSurnames(std::vector<std::string>() << toStdString(addressee.familyName()));
    nc.setGiven(std::vector<std::string>() << toStdString(addressee.givenName()));
    nc.setAdditional(std::vector<std::string>() << toStdString(addressee.additionalName()));
    nc.setPrefixes(std::vector<std::string>() << toStdString(addressee.prefix()));
    nc.setSuffixes(std::vector<std::string>() << toStdString(addressee.suffix()));
    c.setNameComponents(nc);

    c.setNote(toStdString(addressee.note()));
    c.setFreeBusyUrl(toStdString(addressee.custom(QStringLiteral("KOLAB"), QStringLiteral("FreebusyUrl"))));

    if (!addressee.title().isEmpty()) {
        c.setTitles(std::vector<std::string>() << toStdString(addressee.title()));
    }

    Kolab::Affiliation businessAff;
    businessAff.setOrganisation(toStdString(addressee.organization()));
    if (!addressee.department().isEmpty()) {
        Debug() << addressee.department() << addressee.department().toLatin1() << addressee.department().toUtf8();
        businessAff.setOrganisationalUnits(std::vector<std::string>() << toStdString(addressee.department()));
    }

    if (!addressee.logo().isEmpty()) {
        std::string logoMimetype;
        const std::string &logo = fromPicture(addressee.logo(), logoMimetype);
        businessAff.setLogo(logo, logoMimetype);
    }
    if (!addressee.role().isEmpty()) {
        businessAff.setRoles(std::vector<std::string>() << toStdString(addressee.role()));
    }
    const QString &office = addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-Office"));
    if (!office.isEmpty()) {
        Kolab::Address a;
        a.setTypes(Kolab::Address::Work);
        a.setLabel(toStdString(addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-Office"))));
        businessAff.setAddresses(std::vector<Kolab::Address>() << a);
    }

    std::vector<Kolab::Related> relateds;
    const QString &manager = addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-ManagersName"));
    if (!manager.isEmpty()) {
        relateds.push_back(Kolab::Related(Kolab::Related::Text, toStdString(manager), Kolab::Related::Manager));
    }
    const QString &assistant = addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-AssistantsName"));
    if (!assistant.isEmpty()) {
        relateds.push_back(Kolab::Related(Kolab::Related::Text, toStdString(assistant), Kolab::Related::Assistant));
    }
    if (!relateds.empty()) {
        businessAff.setRelateds(relateds);
    }
    if (!(businessAff == Kolab::Affiliation())) {
        c.setAffiliations(std::vector<Kolab::Affiliation>() << businessAff);
    }

    std::vector<Kolab::Url> urls;
    const QUrl url{addressee.url().url()};
    if (!url.isEmpty()) {
        urls.push_back(Kolab::Url(toStdString(url.url())));
    }
    const QString &blogUrl = addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("BlogFeed"));
    if (!blogUrl.isEmpty()) {
        urls.push_back(Kolab::Url(toStdString(blogUrl), Kolab::Url::Blog));
    }
    c.setUrls(urls);

    std::vector<Kolab::Address> addresses;
    prefNum = -1;
    prefCounter = -1;
    foreach (const KContacts::Address &a, addressee.addresses()) {
        Kolab::Address adr;
        bool pref = false;
        adr.setTypes(fromAddressType(a.type(), pref));
        prefCounter++;
        if (pref) {
            prefNum = prefCounter;
        }
        adr.setLabel(toStdString(a.label()));
        adr.setStreet(toStdString(a.street()));
        adr.setLocality(toStdString(a.locality()));
        adr.setRegion(toStdString(a.region()));
        adr.setCode(toStdString(a.postalCode()));
        adr.setCountry(toStdString(a.country()));
        addresses.push_back(adr);
    }
    c.setAddresses(addresses, prefNum);

    if (!addressee.nickName().isEmpty()) {
        c.setNickNames(std::vector<std::string>() << toStdString(addressee.nickName()));
    }

    const QString &spouse = addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-SpousesName"));
    if (!spouse.isEmpty()) {
        c.setRelateds(std::vector<Kolab::Related>() << Kolab::Related(Kolab::Related::Text, toStdString(spouse), Kolab::Related::Spouse));
    }
    c.setBDay(fromDate(addressee.birthday(), true));
    c.setAnniversary(fromDate(QDateTime(QDate::fromString(addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-Anniversary")), Qt::ISODate), {}), true));
    if (!addressee.photo().isEmpty()) {
        std::string mimetype;
        const std::string &photo = fromPicture(addressee.photo(), mimetype);
        c.setPhoto(photo, mimetype);
    }
    //TODO
//     c.setGender();
//     c.setLanguages();

    std::vector <Kolab::Telephone> phones;
    prefNum = -1;
    prefCounter = -1;
    foreach (const KContacts::PhoneNumber &n, addressee.phoneNumbers()) {
        Kolab::Telephone p;
        p.setNumber(toStdString(n.number()));
        bool pref = false;
        p.setTypes(fromPhoneType(n.type(), pref));
        prefCounter++;
        if (pref) {
            prefNum = prefCounter;
        }
        phones.push_back(p);
    }
    c.setTelephones(phones, prefNum);

    const QString &imAddress = addressee.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("X-IMAddress"));
    if (!imAddress.isEmpty()) {
        c.setIMaddresses(std::vector<std::string>() << toStdString(imAddress), 0);
    }

    int prefEmail = -1;
    int count = 0;
    std::vector<Kolab::Email> emails;
    emails.resize(addressee.emails().count());
    foreach (const QString &e, addressee.emails()) {
        if ((prefEmail == -1) && (e == addressee.preferredEmail())) {
            prefEmail = count;
        }
        count++;
        emails.push_back(Kolab::Email(toStdString(e), emailTypesFromStringlist(addressee.custom(QLatin1String("KOLAB"), QString::fromLatin1("EmailTypes%1").arg(e)))));
    }
    c.setEmailAddresses(emails, prefEmail);
    if (addressee.geo().isValid()) {
        c.setGPSpos(std::vector<Kolab::Geo>() << Kolab::Geo(addressee.geo().latitude(), addressee.geo().longitude()));
    }

    Kolab::Crypto crypto;

    const QStringList protocolPrefs = addressee.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("CRYPTOPROTOPREF")).split(',', QString::SkipEmptyParts);
    const uint cryptoFormats = stringListToCryptoMessageFormats(protocolPrefs);
    int formats = 0;
    if (cryptoFormats & InlineOpenPGPFormat) {
        formats |= Kolab::Crypto::PGPinline;
    }
    if (cryptoFormats & OpenPGPMIMEFormat) {
        formats |= Kolab::Crypto::PGPmime;
    }
    if (cryptoFormats & SMIMEFormat) {
        formats |= Kolab::Crypto::SMIME;
    }
    if (cryptoFormats & SMIMEOpaqueFormat) {
        formats |= Kolab::Crypto::SMIMEopaque;
    }
    crypto.setAllowed(formats);

    Kolab::Crypto::CryptoPref signPref = Kolab::Crypto::Ask;
    switch (stringToSigningPreference(addressee.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("CRYPTOSIGNPREF")))) {
    case NeverSign:
        signPref = Kolab::Crypto::Never;
        break;
    case AlwaysSign:
        signPref = Kolab::Crypto::Always;
        break;
    case AlwaysSignIfPossible:
        signPref = Kolab::Crypto::IfPossible;
        break;
    case AlwaysAskForSigning:
    case AskSigningWheneverPossible:
        signPref = Kolab::Crypto::Ask;
        break;
    default:
        signPref = Kolab::Crypto::Ask;
    }
    crypto.setSignPref(signPref);

    Kolab::Crypto::CryptoPref encryptPref = Kolab::Crypto::Ask;
    switch (stringToSigningPreference(addressee.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("CRYPTOENCRYPTPREF")))) {
    case NeverEncrypt:
        encryptPref = Kolab::Crypto::Never;
        break;
    case AlwaysEncrypt:
        encryptPref = Kolab::Crypto::Always;
        break;
    case AlwaysEncryptIfPossible:
        encryptPref = Kolab::Crypto::IfPossible;
        break;
    case AlwaysAskForEncryption:
    case AskWheneverPossible:
        encryptPref = Kolab::Crypto::Ask;
        break;
    default:
        encryptPref = Kolab::Crypto::Ask;
    }
    crypto.setEncryptPref(encryptPref);

    c.setCrypto(crypto);

    //FIXME the keys are most certainly wrong, look at cryptopageplugin.cpp
    std::vector<Kolab::Key> keys;
    const std::string &pgpkey = toStdString(addressee.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("OPENPGPFP")));
    if (!pgpkey.empty()) {
        keys.push_back(Kolab::Key(pgpkey, Kolab::Key::PGP));
    }
    const std::string &smimekey = toStdString(addressee.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("SMIMEFP")));
    if (!smimekey.empty()) {
        keys.push_back(Kolab::Key(smimekey, Kolab::Key::PKCS7_MIME));
    }
    c.setKeys(keys);

    if (!addressee.sound().isEmpty()) {
        Warning() << "sound is not supported";
    }

    const std::string &profession = toStdString(addressee.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("X-Profession")));
    if (!profession.empty()) {
        setCustom(profession, "X-Profession", c);
    }

    const std::string &adrBook = toStdString(addressee.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("X-AddressBook")));
    if (!adrBook.empty()) {
        setCustom(adrBook, "X-AddressBook", c);
    }

    //TODO preserve all custom properties (also such which are unknown to us)

    return c;
}

DistList fromKABC(const KContacts::ContactGroup &cg)
{
    DistList dl;
    dl.setName(toStdString(cg.name()));
    dl.setUid(toStdString(cg.id()));

    std::vector <Kolab::ContactReference > members;
    for (unsigned int i = 0; i < cg.dataCount(); i++) {
        const KContacts::ContactGroup::Data &data = cg.data(i);
        members.push_back(Kolab::ContactReference(Kolab::ContactReference::EmailReference, toStdString(data.email()), toStdString(data.name())));
    }
    for (unsigned int i = 0; i < cg.contactReferenceCount(); i++) {
        const KContacts::ContactGroup::ContactReference &ref = cg.contactReference(i);
        members.push_back(Kolab::ContactReference(Kolab::ContactReference::UidReference, toStdString(ref.uid())));
    }

    if (cg.contactGroupReferenceCount() > 0) {
        qWarning() << "Tried to save contact group references, which should have been resolved already";
    }

    dl.setMembers(members);

    return dl;
}

KContacts::ContactGroup toKABC(const DistList &dl)
{
    KContacts::ContactGroup cg(fromStdString(dl.name()));
    cg.setId(fromStdString(dl.uid()));
    foreach (const Kolab::ContactReference &m, dl.members()) {
        switch (m.type()) {
        case Kolab::ContactReference::EmailReference:
            cg.append(KContacts::ContactGroup::Data(fromStdString(m.name()), fromStdString(m.email())));
            break;
        case Kolab::ContactReference::UidReference:
            cg.append(KContacts::ContactGroup::ContactReference(fromStdString(m.uid())));
            break;
        default:
            Error() << "invalid contact reference";
        }
    }

    return cg;
}
}     //Namespace
} //Namespace
