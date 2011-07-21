#ifndef _NCO_EMAILADDRESS_H_
#define _NCO_EMAILADDRESS_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/contactmedium.h"
namespace Nepomuk {
namespace NCO {
/**
 * An email address. The recommended best practice is to use mailto: 
 * uris for instances of this class. 
 */
class EmailAddress : public NCO::ContactMedium
{
public:
    EmailAddress(Nepomuk::SimpleResource* res)
      : NCO::ContactMedium(res), m_res(res)
    {}

    virtual ~EmailAddress() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress. 
     */
    QString emailAddress() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress. 
     */
    void setEmailAddress(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress. 
     */
    void addEmailAddress(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#EmailAddress", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
