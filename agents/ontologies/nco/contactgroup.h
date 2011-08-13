#ifndef _NCO_CONTACTGROUP_H_
#define _NCO_CONTACTGROUP_H_

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
 * A group of Contacts. Could be used to express a group in an addressbook 
 * or on a contact list of an IM application. One contact can belong 
 * to many groups. 
 */
class ContactGroup
{
public:
    ContactGroup(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~ContactGroup() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactGroupName. 
     * The name of the contact group. This property was NOT defined 
     * in the VCARD standard. See documentation of the 'ContactGroup' 
     * class for details 
     */
    QString contactGroupName() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactGroupName", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactGroupName", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactGroupName. 
     * The name of the contact group. This property was NOT defined 
     * in the VCARD standard. See documentation of the 'ContactGroup' 
     * class for details 
     */
    void setContactGroupName(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactGroupName", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactGroupName. 
     * The name of the contact group. This property was NOT defined 
     * in the VCARD standard. See documentation of the 'ContactGroup' 
     * class for details 
     */
    void addContactGroupName(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactGroupName", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#ContactGroup", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
