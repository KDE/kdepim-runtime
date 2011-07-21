#ifndef _NCO_ROLE_H_
#define _NCO_ROLE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

namespace Nepomuk {
namespace NCO {
/**
 * A role played by a contact. Contacts that denote people, can 
 * have many roles (e.g. see the hasAffiliation property and Affiliation 
 * class). Contacts that denote Organizations or other Agents 
 * usually have one role. Each role can introduce additional contact 
 * media. 
 */
class Role
{
public:
    Role(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Role() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasEmailAddress. 
     * An address for electronic mail communication with the object 
     * specified by this contact. An equivalent of the 'EMAIL' property 
     * as defined in RFC 2426 Sec. 3.3.1. 
     */
    QList<QUrl> hasEmailAddresses() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasEmailAddress", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasEmailAddress. 
     * An address for electronic mail communication with the object 
     * specified by this contact. An equivalent of the 'EMAIL' property 
     * as defined in RFC 2426 Sec. 3.3.1. 
     */
    void setHasEmailAddresses(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasEmailAddress", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasEmailAddress. 
     * An address for electronic mail communication with the object 
     * specified by this contact. An equivalent of the 'EMAIL' property 
     * as defined in RFC 2426 Sec. 3.3.1. 
     */
    void addHasEmailAddress(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasEmailAddress", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#end. 
     * End datetime for the role, such as: the datetime of leaving a 
     * project or organization, datetime of ending employment, datetime 
     * of divorce. If absent or set to a date in the future, the role is 
     * currently active. 
     */
    QDateTime end() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#end", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#end", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#end. 
     * End datetime for the role, such as: the datetime of leaving a 
     * project or organization, datetime of ending employment, datetime 
     * of divorce. If absent or set to a date in the future, the role is 
     * currently active. 
     */
    void setEnd(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#end", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#end. 
     * End datetime for the role, such as: the datetime of leaving a 
     * project or organization, datetime of ending employment, datetime 
     * of divorce. If absent or set to a date in the future, the role is 
     * currently active. 
     */
    void addEnd(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#end", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#websiteUrl. 
     * A url of a website. 
     */
    QList<QUrl> websiteUrls() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#websiteUrl", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#websiteUrl. 
     * A url of a website. 
     */
    void setWebsiteUrls(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#websiteUrl", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#websiteUrl. 
     * A url of a website. 
     */
    void addWebsiteUrl(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#websiteUrl", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#url. 
     * A uniform resource locator associated with the given role of 
     * a Contact. Inspired by the 'URL' property defined in RFC 2426 
     * Sec. 3.6.8. 
     */
    QList<QUrl> urls() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#url", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#url. 
     * A uniform resource locator associated with the given role of 
     * a Contact. Inspired by the 'URL' property defined in RFC 2426 
     * Sec. 3.6.8. 
     */
    void setUrls(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#url", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#url. 
     * A uniform resource locator associated with the given role of 
     * a Contact. Inspired by the 'URL' property defined in RFC 2426 
     * Sec. 3.6.8. 
     */
    void addUrl(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#url", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPostalAddress. 
     * The default Address for a Contact. An equivalent of the 'ADR' 
     * property as defined in RFC 2426 Sec. 3.2.1. 
     */
    QList<QUrl> hasPostalAddresses() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPostalAddress", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPostalAddress. 
     * The default Address for a Contact. An equivalent of the 'ADR' 
     * property as defined in RFC 2426 Sec. 3.2.1. 
     */
    void setHasPostalAddresses(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPostalAddress", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPostalAddress. 
     * The default Address for a Contact. An equivalent of the 'ADR' 
     * property as defined in RFC 2426 Sec. 3.2.1. 
     */
    void addHasPostalAddress(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPostalAddress", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#blogUrl. 
     * A Blog url. 
     */
    QList<QUrl> blogUrls() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#blogUrl", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#blogUrl. 
     * A Blog url. 
     */
    void setBlogUrls(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#blogUrl", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#blogUrl. 
     * A Blog url. 
     */
    void addBlogUrl(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#blogUrl", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMAccount. 
     * Indicates that an Instant Messaging account owned by an entity 
     * represented by this contact. 
     */
    QList<QUrl> hasIMAccounts() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMAccount", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMAccount. 
     * Indicates that an Instant Messaging account owned by an entity 
     * represented by this contact. 
     */
    void setHasIMAccounts(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMAccount", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMAccount. 
     * Indicates that an Instant Messaging account owned by an entity 
     * represented by this contact. 
     */
    void addHasIMAccount(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMAccount", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPhoneNumber. 
     * A number for telephony communication with the object represented 
     * by this Contact. An equivalent of the 'TEL' property defined 
     * in RFC 2426 Sec. 3.3.1 
     */
    QList<QUrl> hasPhoneNumbers() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPhoneNumber", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPhoneNumber. 
     * A number for telephony communication with the object represented 
     * by this Contact. An equivalent of the 'TEL' property defined 
     * in RFC 2426 Sec. 3.3.1 
     */
    void setHasPhoneNumbers(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPhoneNumber", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPhoneNumber. 
     * A number for telephony communication with the object represented 
     * by this Contact. An equivalent of the 'TEL' property defined 
     * in RFC 2426 Sec. 3.3.1 
     */
    void addHasPhoneNumber(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasPhoneNumber", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#foafUrl. 
     * The URL of the FOAF file. 
     */
    QList<QUrl> foafUrls() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#foafUrl", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#foafUrl. 
     * The URL of the FOAF file. 
     */
    void setFoafUrls(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#foafUrl", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#foafUrl. 
     * The URL of the FOAF file. 
     */
    void addFoafUrl(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#foafUrl", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#start. 
     * Start datetime for the role, such as: the datetime of joining 
     * a project or organization, datetime of starting employment, 
     * datetime of marriage 
     */
    QDateTime start() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#start", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#start", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#start. 
     * Start datetime for the role, such as: the datetime of joining 
     * a project or organization, datetime of starting employment, 
     * datetime of marriage 
     */
    void setStart(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#start", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#start. 
     * Start datetime for the role, such as: the datetime of joining 
     * a project or organization, datetime of starting employment, 
     * datetime of marriage 
     */
    void addStart(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#start", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasContactMedium. 
     * A superProperty for all properties linking a Contact to an instance 
     * of a contact medium. 
     */
    QList<QUrl> hasContactMediums() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasContactMedium", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasContactMedium. 
     * A superProperty for all properties linking a Contact to an instance 
     * of a contact medium. 
     */
    void setHasContactMediums(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasContactMedium", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasContactMedium. 
     * A superProperty for all properties linking a Contact to an instance 
     * of a contact medium. 
     */
    void addHasContactMedium(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasContactMedium", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Role", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
