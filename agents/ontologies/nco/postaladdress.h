#ifndef _NCO_POSTALADDRESS_H_
#define _NCO_POSTALADDRESS_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nco/contactmedium.h"
namespace Nepomuk {
namespace NCO {
/**
 * A postal address. A class aggregating the various parts of a 
 * value for the 'ADR' property as defined in RFC 2426 Sec. 3.2.1. 
 */
class PostalAddress : public NCO::ContactMedium
{
public:
    PostalAddress(Nepomuk::SimpleResource* res)
      : NCO::ContactMedium(res), m_res(res)
    {}

    virtual ~PostalAddress() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#streetAddress. 
     * The streed address. Inspired by the third part of the value of 
     * the 'ADR' property as defined in RFC 2426, sec. 3.2.1 
     */
    QString streetAddress() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#streetAddress", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#streetAddress", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#streetAddress. 
     * The streed address. Inspired by the third part of the value of 
     * the 'ADR' property as defined in RFC 2426, sec. 3.2.1 
     */
    void setStreetAddress(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#streetAddress", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#streetAddress. 
     * The streed address. Inspired by the third part of the value of 
     * the 'ADR' property as defined in RFC 2426, sec. 3.2.1 
     */
    void addStreetAddress(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#streetAddress", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#pobox. 
     * Post office box. This is the first part of the value of the 'ADR' 
     * property as defined in RFC 2426, sec. 3.2.1 
     */
    QString pobox() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#pobox", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#pobox", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#pobox. 
     * Post office box. This is the first part of the value of the 'ADR' 
     * property as defined in RFC 2426, sec. 3.2.1 
     */
    void setPobox(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#pobox", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#pobox. 
     * Post office box. This is the first part of the value of the 'ADR' 
     * property as defined in RFC 2426, sec. 3.2.1 
     */
    void addPobox(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#pobox", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#extendedAddress. 
     * An extended part of an address. This field might be used to express 
     * parts of an address that aren't include in the name of the Contact 
     * but also aren't part of the actual location. Usually the streed 
     * address and following fields are enough for a postal letter 
     * to arrive. Examples may include ('University of California 
     * Campus building 45', 'Sears Tower 34th floor' etc.) Inspired 
     * by the second part of the value of the 'ADR' property as defined 
     * in RFC 2426, sec. 3.2.1 
     */
    QString extendedAddress() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#extendedAddress", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#extendedAddress", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#extendedAddress. 
     * An extended part of an address. This field might be used to express 
     * parts of an address that aren't include in the name of the Contact 
     * but also aren't part of the actual location. Usually the streed 
     * address and following fields are enough for a postal letter 
     * to arrive. Examples may include ('University of California 
     * Campus building 45', 'Sears Tower 34th floor' etc.) Inspired 
     * by the second part of the value of the 'ADR' property as defined 
     * in RFC 2426, sec. 3.2.1 
     */
    void setExtendedAddress(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#extendedAddress", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#extendedAddress. 
     * An extended part of an address. This field might be used to express 
     * parts of an address that aren't include in the name of the Contact 
     * but also aren't part of the actual location. Usually the streed 
     * address and following fields are enough for a postal letter 
     * to arrive. Examples may include ('University of California 
     * Campus building 45', 'Sears Tower 34th floor' etc.) Inspired 
     * by the second part of the value of the 'ADR' property as defined 
     * in RFC 2426, sec. 3.2.1 
     */
    void addExtendedAddress(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#extendedAddress", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#region. 
     * Region. Inspired by the fifth part of the value of the 'ADR' property 
     * as defined in RFC 2426, sec. 3.2.1 
     */
    QString region() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#region", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#region", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#region. 
     * Region. Inspired by the fifth part of the value of the 'ADR' property 
     * as defined in RFC 2426, sec. 3.2.1 
     */
    void setRegion(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#region", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#region. 
     * Region. Inspired by the fifth part of the value of the 'ADR' property 
     * as defined in RFC 2426, sec. 3.2.1 
     */
    void addRegion(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#region", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#addressLocation. 
     * The geographical location of a postal address. 
     */
    QUrl addressLocation() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#addressLocation", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#addressLocation", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#addressLocation. 
     * The geographical location of a postal address. 
     */
    void setAddressLocation(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#addressLocation", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#addressLocation. 
     * The geographical location of a postal address. 
     */
    void addAddressLocation(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#addressLocation", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#locality. 
     * Locality or City. Inspired by the fourth part of the value of 
     * the 'ADR' property as defined in RFC 2426, sec. 3.2.1 
     */
    QString locality() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#locality", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#locality", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#locality. 
     * Locality or City. Inspired by the fourth part of the value of 
     * the 'ADR' property as defined in RFC 2426, sec. 3.2.1 
     */
    void setLocality(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#locality", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#locality. 
     * Locality or City. Inspired by the fourth part of the value of 
     * the 'ADR' property as defined in RFC 2426, sec. 3.2.1 
     */
    void addLocality(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#locality", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#country. 
     * A part of an address specyfing the country. Inspired by the seventh 
     * part of the value of the 'ADR' property as defined in RFC 2426, 
     * sec. 3.2.1 
     */
    QString country() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#country", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#country", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#country. 
     * A part of an address specyfing the country. Inspired by the seventh 
     * part of the value of the 'ADR' property as defined in RFC 2426, 
     * sec. 3.2.1 
     */
    void setCountry(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#country", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#country. 
     * A part of an address specyfing the country. Inspired by the seventh 
     * part of the value of the 'ADR' property as defined in RFC 2426, 
     * sec. 3.2.1 
     */
    void addCountry(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#country", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#postalcode. 
     * Postal Code. Inspired by the sixth part of the value of the 'ADR' 
     * property as defined in RFC 2426, sec. 3.2.1 
     */
    QString postalcode() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#postalcode", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#postalcode", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#postalcode. 
     * Postal Code. Inspired by the sixth part of the value of the 'ADR' 
     * property as defined in RFC 2426, sec. 3.2.1 
     */
    void setPostalcode(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#postalcode", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#postalcode. 
     * Postal Code. Inspired by the sixth part of the value of the 'ADR' 
     * property as defined in RFC 2426, sec. 3.2.1 
     */
    void addPostalcode(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#postalcode", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#PostalAddress", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
