#ifndef _NCO_CONTACTMEDIUM_H_
#define _NCO_CONTACTMEDIUM_H_

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
 * A superclass for all contact media - ways to contact an entity 
 * represented by a Contact instance. Some of the subclasses of 
 * this class (the various kinds of telephone numbers and postal 
 * addresses) have been inspired by the values of the TYPE parameter 
 * of ADR and TEL properties defined in RFC 2426 sec. 3.2.1. and 
 * 3.3.1 respectively. Each value is represented by an appropriate 
 * subclass with two major exceptions TYPE=home and TYPE=work. 
 * They are to be expressed by the roles these contact media are 
 * attached to i.e. contact media with TYPE=home parameter are 
 * to be attached to the default role (nco:Contact or nco:PersonContact), 
 * whereas media with TYPE=work parameter should be attached 
 * to nco:Affiliation or nco:OrganizationContact. 
 */
class ContactMedium
{
public:
    ContactMedium(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~ContactMedium() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactMediumComment. 
     * A comment about the contact medium. (Deprecated in favor of 
     * nie:comment or nao:description - based on the context) 
     */
    QStringList contactMediumComments() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactMediumComment", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactMediumComment. 
     * A comment about the contact medium. (Deprecated in favor of 
     * nie:comment or nao:description - based on the context) 
     */
    void setContactMediumComments(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactMediumComment", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactMediumComment. 
     * A comment about the contact medium. (Deprecated in favor of 
     * nie:comment or nao:description - based on the context) 
     */
    void addContactMediumComment(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactMediumComment", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#ContactMedium", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
