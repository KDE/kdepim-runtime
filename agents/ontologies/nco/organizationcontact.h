#ifndef _NCO_ORGANIZATIONCONTACT_H_
#define _NCO_ORGANIZATIONCONTACT_H_

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
 * A Contact that denotes on Organization. 
 */
class OrganizationContact : public NCO::Contact
{
public:
    OrganizationContact(Nepomuk::SimpleResource* res)
      : NCO::Contact(res), m_res(res)
    {}

    virtual ~OrganizationContact() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#logo. 
     * Logo of a company. Inspired by the LOGO property defined in RFC 
     * 2426 sec. 3.5.3 
     */
    QList<QUrl> logos() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#logo", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#logo. 
     * Logo of a company. Inspired by the LOGO property defined in RFC 
     * 2426 sec. 3.5.3 
     */
    void setLogos(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#logo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#logo. 
     * Logo of a company. Inspired by the LOGO property defined in RFC 
     * 2426 sec. 3.5.3 
     */
    void addLogo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#logo", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#OrganizationContact", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
