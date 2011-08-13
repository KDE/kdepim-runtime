#ifndef _NCO_PERSONCONTACT_H_
#define _NCO_PERSONCONTACT_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nco/contact.h"
namespace Nepomuk {
namespace NCO {
/**
 * A Contact that denotes a Person. A person can have multiple Affiliations. 
 */
class PersonContact : public NCO::Contact
{
public:
    PersonContact(Nepomuk::SimpleResource* res)
      : NCO::Contact(res), m_res(res)
    {}

    virtual ~PersonContact() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameFamily. 
     * The family name of an Object represented by this Contact. These 
     * applies to people that have more than one given name. The 'first' 
     * one is considered 'the' given name (see nameGiven) property. 
     * All additional ones are considered 'additional' names. The 
     * name inherited from parents is the 'family name'. e.g. For Dr. 
     * John Phil Paul Stevenson Jr. M.D. A.C.P. we have contact with: 
     * honorificPrefix: 'Dr.', nameGiven: 'John', nameAdditional: 
     * 'Phil', nameAdditional: 'Paul', nameFamily: 'Stevenson', 
     * honorificSuffix: 'Jr.', honorificSuffix: 'M.D.', honorificSuffix: 
     * 'A.C.P.'. These properties form an equivalent of the compound 
     * 'N' property as defined in RFC 2426 Sec. 3.1.2 
     */
    QString nameFamily() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameFamily", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameFamily", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameFamily. 
     * The family name of an Object represented by this Contact. These 
     * applies to people that have more than one given name. The 'first' 
     * one is considered 'the' given name (see nameGiven) property. 
     * All additional ones are considered 'additional' names. The 
     * name inherited from parents is the 'family name'. e.g. For Dr. 
     * John Phil Paul Stevenson Jr. M.D. A.C.P. we have contact with: 
     * honorificPrefix: 'Dr.', nameGiven: 'John', nameAdditional: 
     * 'Phil', nameAdditional: 'Paul', nameFamily: 'Stevenson', 
     * honorificSuffix: 'Jr.', honorificSuffix: 'M.D.', honorificSuffix: 
     * 'A.C.P.'. These properties form an equivalent of the compound 
     * 'N' property as defined in RFC 2426 Sec. 3.1.2 
     */
    void setNameFamily(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameFamily", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameFamily. 
     * The family name of an Object represented by this Contact. These 
     * applies to people that have more than one given name. The 'first' 
     * one is considered 'the' given name (see nameGiven) property. 
     * All additional ones are considered 'additional' names. The 
     * name inherited from parents is the 'family name'. e.g. For Dr. 
     * John Phil Paul Stevenson Jr. M.D. A.C.P. we have contact with: 
     * honorificPrefix: 'Dr.', nameGiven: 'John', nameAdditional: 
     * 'Phil', nameAdditional: 'Paul', nameFamily: 'Stevenson', 
     * honorificSuffix: 'Jr.', honorificSuffix: 'M.D.', honorificSuffix: 
     * 'A.C.P.'. These properties form an equivalent of the compound 
     * 'N' property as defined in RFC 2426 Sec. 3.1.2 
     */
    void addNameFamily(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameFamily", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hobby. 
     * A hobby associated with a PersonContact. This property can 
     * be used to express hobbies and interests. 
     */
    QStringList hobbys() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hobby", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hobby. 
     * A hobby associated with a PersonContact. This property can 
     * be used to express hobbies and interests. 
     */
    void setHobbys(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hobby", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hobby. 
     * A hobby associated with a PersonContact. This property can 
     * be used to express hobbies and interests. 
     */
    void addHobby(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hobby", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#gender. 
     * Gender of the given contact. 
     */
    QUrl gender() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#gender", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#gender", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#gender. 
     * Gender of the given contact. 
     */
    void setGender(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#gender", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#gender. 
     * Gender of the given contact. 
     */
    void addGender(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#gender", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameAdditional. 
     * Additional given name of an object represented by this contact. 
     * See documentation for 'nameFamily' property for details. 
     */
    QStringList nameAdditionals() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameAdditional", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameAdditional. 
     * Additional given name of an object represented by this contact. 
     * See documentation for 'nameFamily' property for details. 
     */
    void setNameAdditionals(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameAdditional", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameAdditional. 
     * Additional given name of an object represented by this contact. 
     * See documentation for 'nameFamily' property for details. 
     */
    void addNameAdditional(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameAdditional", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasAffiliation. 
     * Links a PersonContact with an Affiliation. 
     */
    QList<QUrl> hasAffiliations() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasAffiliation", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasAffiliation. 
     * Links a PersonContact with an Affiliation. 
     */
    void setHasAffiliations(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasAffiliation", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasAffiliation. 
     * Links a PersonContact with an Affiliation. 
     */
    void addHasAffiliation(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasAffiliation", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificPrefix. 
     * A prefix for the name of the object represented by this Contact. 
     * See documentation for the 'nameFamily' property for details. 
     */
    QStringList nameHonorificPrefixs() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificPrefix", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificPrefix. 
     * A prefix for the name of the object represented by this Contact. 
     * See documentation for the 'nameFamily' property for details. 
     */
    void setNameHonorificPrefixs(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificPrefix", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificPrefix. 
     * A prefix for the name of the object represented by this Contact. 
     * See documentation for the 'nameFamily' property for details. 
     */
    void addNameHonorificPrefix(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificPrefix", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificSuffix. 
     * A suffix for the name of the Object represented by the given object. 
     * See documentation for the 'nameFamily' for details. 
     */
    QStringList nameHonorificSuffixs() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificSuffix", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificSuffix. 
     * A suffix for the name of the Object represented by the given object. 
     * See documentation for the 'nameFamily' for details. 
     */
    void setNameHonorificSuffixs(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificSuffix", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificSuffix. 
     * A suffix for the name of the Object represented by the given object. 
     * See documentation for the 'nameFamily' for details. 
     */
    void addNameHonorificSuffix(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameHonorificSuffix", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameGiven. 
     * The given name for the object represented by this Contact. See 
     * documentation for 'nameFamily' property for details. 
     */
    QString nameGiven() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameGiven", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameGiven", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameGiven. 
     * The given name for the object represented by this Contact. See 
     * documentation for 'nameFamily' property for details. 
     */
    void setNameGiven(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameGiven", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameGiven. 
     * The given name for the object represented by this Contact. See 
     * documentation for 'nameFamily' property for details. 
     */
    void addNameGiven(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nameGiven", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#PersonContact", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
