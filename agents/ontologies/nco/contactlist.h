#ifndef _NCO_CONTACTLIST_H_
#define _NCO_CONTACTLIST_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

namespace Nepomuk {
namespace NCO {
/**
 * A contact list, this class represents an addressbook or a contact 
 * list of an IM application. Contacts inside a contact list can 
 * belong to contact groups. 
 */
class ContactList
{
public:
    ContactList(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~ContactList() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#containsContact. 
     * A property used to group contacts into contact groups. This 
     * property was NOT defined in the VCARD standard. See documentation 
     * for the 'ContactList' class for details 
     */
    QList<QUrl> containsContacts() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#containsContact", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#containsContact. 
     * A property used to group contacts into contact groups. This 
     * property was NOT defined in the VCARD standard. See documentation 
     * for the 'ContactList' class for details 
     */
    void setContainsContacts(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#containsContact", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#containsContact. 
     * A property used to group contacts into contact groups. This 
     * property was NOT defined in the VCARD standard. See documentation 
     * for the 'ContactList' class for details 
     */
    void addContainsContact(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#containsContact", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#ContactList", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
