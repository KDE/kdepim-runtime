#ifndef _NCAL_UNIONOFTIMEZONEOBSERVANCEEVENTJOURNALTIMEZONETODO_H_
#define _NCAL_UNIONOFTIMEZONEOBSERVANCEEVENTJOURNALTIMEZONETODO_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "ncal/unionparentclass.h"
namespace Nepomuk {
namespace NCAL {
/**
 * 
 */
class UnionOfTimezoneObservanceEventJournalTimezoneTodo : public NCAL::UnionParentClass
{
public:
    UnionOfTimezoneObservanceEventJournalTimezoneTodo(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfTimezoneObservanceEventJournalTimezoneTodo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rdate. 
     * This property defines the list of date/times for a recurrence 
     * set. Inspired by RFC 2445 sec. 4.8.5.3. Note that RFC allows 
     * both DATE, DATE-TIME and PERIOD values for this property. That's 
     * why the range has been set to NcalTimeEntity. 
     */
    QList<QUrl> rdates() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rdate", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rdate. 
     * This property defines the list of date/times for a recurrence 
     * set. Inspired by RFC 2445 sec. 4.8.5.3. Note that RFC allows 
     * both DATE, DATE-TIME and PERIOD values for this property. That's 
     * why the range has been set to NcalTimeEntity. 
     */
    void setRdates(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rdate", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rdate. 
     * This property defines the list of date/times for a recurrence 
     * set. Inspired by RFC 2445 sec. 4.8.5.3. Note that RFC allows 
     * both DATE, DATE-TIME and PERIOD values for this property. That's 
     * why the range has been set to NcalTimeEntity. 
     */
    void addRdate(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rdate", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rrule. 
     * This property defines a rule or repeating pattern for recurring 
     * events, to-dos, or time zone definitions. sec. 4.8.5.4 
     */
    QList<QUrl> rrules() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rrule", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rrule. 
     * This property defines a rule or repeating pattern for recurring 
     * events, to-dos, or time zone definitions. sec. 4.8.5.4 
     */
    void setRrules(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rrule", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rrule. 
     * This property defines a rule or repeating pattern for recurring 
     * events, to-dos, or time zone definitions. sec. 4.8.5.4 
     */
    void addRrule(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#rrule", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfTimezoneObservanceEventJournalTimezoneTodo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
