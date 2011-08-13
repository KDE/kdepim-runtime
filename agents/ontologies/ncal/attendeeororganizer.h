#ifndef _NCAL_ATTENDEEORORGANIZER_H_
#define _NCAL_ATTENDEEORORGANIZER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

namespace Nepomuk {
namespace NCAL {
/**
 * A common superclass for ncal:Attendee and ncal:Organizer. 
 */
class AttendeeOrOrganizer
{
public:
    AttendeeOrOrganizer(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~AttendeeOrOrganizer() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dir. 
     * Specifies a reference to a directory entry associated with 
     * the calendar user specified by the property. Inspired by RFC 
     * 2445 sec. 4.2.6. Originally the data type of the value of this 
     * parameter was URI (Usually an LDAP URI). This has been expressed 
     * as rdfs:resource. 
     */
    QList<QUrl> dirs() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dir", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dir. 
     * Specifies a reference to a directory entry associated with 
     * the calendar user specified by the property. Inspired by RFC 
     * 2445 sec. 4.2.6. Originally the data type of the value of this 
     * parameter was URI (Usually an LDAP URI). This has been expressed 
     * as rdfs:resource. 
     */
    void setDirs(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dir", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dir. 
     * Specifies a reference to a directory entry associated with 
     * the calendar user specified by the property. Inspired by RFC 
     * 2445 sec. 4.2.6. Originally the data type of the value of this 
     * parameter was URI (Usually an LDAP URI). This has been expressed 
     * as rdfs:resource. 
     */
    void addDir(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dir", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#involvedContact. 
     * A contact of the Attendee or the organizer involved in an event 
     * or other calendar entity. This property has been introduced 
     * to express the actual value of the ATTENDEE and ORGANIZER properties. 
     * The contact will also represent the CN parameter of those properties. 
     * See documentation of ncal:attendee or ncal:organizer for 
     * more details. 
     */
    QList<QUrl> involvedContacts() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#involvedContact", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#involvedContact. 
     * A contact of the Attendee or the organizer involved in an event 
     * or other calendar entity. This property has been introduced 
     * to express the actual value of the ATTENDEE and ORGANIZER properties. 
     * The contact will also represent the CN parameter of those properties. 
     * See documentation of ncal:attendee or ncal:organizer for 
     * more details. 
     */
    void setInvolvedContacts(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#involvedContact", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#involvedContact. 
     * A contact of the Attendee or the organizer involved in an event 
     * or other calendar entity. This property has been introduced 
     * to express the actual value of the ATTENDEE and ORGANIZER properties. 
     * The contact will also represent the CN parameter of those properties. 
     * See documentation of ncal:attendee or ncal:organizer for 
     * more details. 
     */
    void addInvolvedContact(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#involvedContact", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sentBy. 
     * To specify the calendar user that is acting on behalf of the calendar 
     * user specified by the property. Inspired by RFC 2445 sec. 4.2.18. 
     * The original data type of this property was a mailto: URI. This 
     * has been changed to nco:Contact to promote integration between 
     * NCO and NCAL. 
     */
    QList<QUrl> sentBys() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sentBy", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sentBy. 
     * To specify the calendar user that is acting on behalf of the calendar 
     * user specified by the property. Inspired by RFC 2445 sec. 4.2.18. 
     * The original data type of this property was a mailto: URI. This 
     * has been changed to nco:Contact to promote integration between 
     * NCO and NCAL. 
     */
    void setSentBys(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sentBy", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sentBy. 
     * To specify the calendar user that is acting on behalf of the calendar 
     * user specified by the property. Inspired by RFC 2445 sec. 4.2.18. 
     * The original data type of this property was a mailto: URI. This 
     * has been changed to nco:Contact to promote integration between 
     * NCO and NCAL. 
     */
    void addSentBy(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sentBy", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#AttendeeOrOrganizer", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
