#ifndef _NCO_AFFILIATION_H_
#define _NCO_AFFILIATION_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/role.h"
namespace Nepomuk {
namespace NCO {
/**
 * Aggregates three properties defined in RFC2426. Originally 
 * all three were attached directly to a person. One person could 
 * have only one title and one role within one organization. This 
 * class is intended to lift this limitation. 
 */
class Affiliation : public NCO::Role
{
public:
    Affiliation(Nepomuk::SimpleResource* res)
      : NCO::Role(res), m_res(res)
    {}

    virtual ~Affiliation() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#org. 
     * Name of an organization or a unit within an organization the 
     * object represented by a Contact is associated with. An equivalent 
     * of the 'ORG' property defined in RFC 2426 Sec. 3.5.5 
     */
    QUrl org() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#org", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#org", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#org. 
     * Name of an organization or a unit within an organization the 
     * object represented by a Contact is associated with. An equivalent 
     * of the 'ORG' property defined in RFC 2426 Sec. 3.5.5 
     */
    void setOrg(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#org", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#org. 
     * Name of an organization or a unit within an organization the 
     * object represented by a Contact is associated with. An equivalent 
     * of the 'ORG' property defined in RFC 2426 Sec. 3.5.5 
     */
    void addOrg(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#org", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#department. 
     * Department. The organizational unit within the organization. 
     */
    QStringList departments() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#department", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#department. 
     * Department. The organizational unit within the organization. 
     */
    void setDepartments(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#department", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#department. 
     * Department. The organizational unit within the organization. 
     */
    void addDepartment(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#department", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#title. 
     * The official title the object represented by this contact in 
     * an organization. E.g. 'CEO', 'Director, Research and Development', 
     * 'Junior Software Developer/Analyst' etc. An equivalent of 
     * the 'TITLE' property defined in RFC 2426 Sec. 3.5.1 
     */
    QString title() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#title", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#title", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#title. 
     * The official title the object represented by this contact in 
     * an organization. E.g. 'CEO', 'Director, Research and Development', 
     * 'Junior Software Developer/Analyst' etc. An equivalent of 
     * the 'TITLE' property defined in RFC 2426 Sec. 3.5.1 
     */
    void setTitle(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#title", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#title. 
     * The official title the object represented by this contact in 
     * an organization. E.g. 'CEO', 'Director, Research and Development', 
     * 'Junior Software Developer/Analyst' etc. An equivalent of 
     * the 'TITLE' property defined in RFC 2426 Sec. 3.5.1 
     */
    void addTitle(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#title", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#role. 
     * Role an object represented by this contact represents in the 
     * organization. This might include 'Programmer', 'Manager', 
     * 'Sales Representative'. Be careful to avoid confusion with 
     * the title property. An equivalent of the 'ROLE' property as 
     * defined in RFC 2426. Sec. 3.5.2. Note the difference between 
     * nco:Role class and nco:role property. 
     */
    QStringList roles() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#role", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#role. 
     * Role an object represented by this contact represents in the 
     * organization. This might include 'Programmer', 'Manager', 
     * 'Sales Representative'. Be careful to avoid confusion with 
     * the title property. An equivalent of the 'ROLE' property as 
     * defined in RFC 2426. Sec. 3.5.2. Note the difference between 
     * nco:Role class and nco:role property. 
     */
    void setRoles(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#role", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#role. 
     * Role an object represented by this contact represents in the 
     * organization. This might include 'Programmer', 'Manager', 
     * 'Sales Representative'. Be careful to avoid confusion with 
     * the title property. An equivalent of the 'ROLE' property as 
     * defined in RFC 2426. Sec. 3.5.2. Note the difference between 
     * nco:Role class and nco:role property. 
     */
    void addRole(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#role", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Affiliation", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
