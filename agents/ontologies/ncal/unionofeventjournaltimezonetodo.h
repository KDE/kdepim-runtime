#ifndef _NCAL_UNIONOFEVENTJOURNALTIMEZONETODO_H_
#define _NCAL_UNIONOFEVENTJOURNALTIMEZONETODO_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "ncal/unionparentclass.h"
namespace Nepomuk {
namespace NCAL {
/**
 * 
 */
class UnionOfEventJournalTimezoneTodo : public NCAL::UnionParentClass
{
public:
    UnionOfEventJournalTimezoneTodo(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfEventJournalTimezoneTodo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceId. 
     * This property is used in conjunction with the "UID" and "SEQUENCE" 
     * property to identify a specific instance of a recurring "VEVENT", 
     * "VTODO" or "VJOURNAL" calendar component. The property value 
     * is the effective value of the "DTSTART" property of the recurrence 
     * instance. Inspired by the RFC 2445 sec. 4.8.4.4 
     */
    QUrl recurrenceId() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceId", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceId", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceId. 
     * This property is used in conjunction with the "UID" and "SEQUENCE" 
     * property to identify a specific instance of a recurring "VEVENT", 
     * "VTODO" or "VJOURNAL" calendar component. The property value 
     * is the effective value of the "DTSTART" property of the recurrence 
     * instance. Inspired by the RFC 2445 sec. 4.8.4.4 
     */
    void setRecurrenceId(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceId", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceId. 
     * This property is used in conjunction with the "UID" and "SEQUENCE" 
     * property to identify a specific instance of a recurring "VEVENT", 
     * "VTODO" or "VJOURNAL" calendar component. The property value 
     * is the effective value of the "DTSTART" property of the recurrence 
     * instance. Inspired by the RFC 2445 sec. 4.8.4.4 
     */
    void addRecurrenceId(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceId", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#lastModified. 
     * The property specifies the date and time that the information 
     * associated with the calendar component was last revised in 
     * the calendar store. Note: This is analogous to the modification 
     * date and time for a file in the file system. Inspired by RFC 2445 
     * sec. 4.8.7.3. Note that the RFC allows ONLY UTC time values for 
     * this property. 
     */
    QDateTime lastModified() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#lastModified", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#lastModified", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#lastModified. 
     * The property specifies the date and time that the information 
     * associated with the calendar component was last revised in 
     * the calendar store. Note: This is analogous to the modification 
     * date and time for a file in the file system. Inspired by RFC 2445 
     * sec. 4.8.7.3. Note that the RFC allows ONLY UTC time values for 
     * this property. 
     */
    void setLastModified(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#lastModified", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#lastModified. 
     * The property specifies the date and time that the information 
     * associated with the calendar component was last revised in 
     * the calendar store. Note: This is analogous to the modification 
     * date and time for a file in the file system. Inspired by RFC 2445 
     * sec. 4.8.7.3. Note that the RFC allows ONLY UTC time values for 
     * this property. 
     */
    void addLastModified(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#lastModified", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exdate. 
     * This property defines the list of date/time exceptions for 
     * a recurring calendar component. Inspired by RFC 2445 sec. 4.8.5.1 
     */
    QList<QUrl> exdates() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exdate", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exdate. 
     * This property defines the list of date/time exceptions for 
     * a recurring calendar component. Inspired by RFC 2445 sec. 4.8.5.1 
     */
    void setExdates(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exdate", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exdate. 
     * This property defines the list of date/time exceptions for 
     * a recurring calendar component. Inspired by RFC 2445 sec. 4.8.5.1 
     */
    void addExdate(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exdate", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfEventJournalTimezoneTodo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
