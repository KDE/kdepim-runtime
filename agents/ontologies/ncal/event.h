#ifndef _NCAL_EVENT_H_
#define _NCAL_EVENT_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "ncal/unionofeventjournaltimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusytimezonetodo.h"
#include "ncal/unionofalarmeventjournaltodo.h"
#include "ncal/unionoftimezoneobservanceeventjournaltimezonetodo.h"
#include "ncal/unionofalarmeventfreebusyjournaltodo.h"
#include "ncal/unionofeventfreebusyjournaltodo.h"
#include "ncal/unionofalarmeventfreebusytodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusyjournaltimezonetodo.h"
#include "ncal/unionofeventfreebusy.h"
#include "ncal/unionofeventjournaltodo.h"
#include "ncal/unionofeventtodo.h"
#include "ncal/unionofalarmeventtodo.h"
namespace Nepomuk {
namespace NCAL {
/**
 * Provide a grouping of component properties that describe an 
 * event. 
 */
class Event : public NCAL::UnionOfEventJournalTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo, public NCAL::UnionOfAlarmEventJournalTodo, public NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo, public NCAL::UnionOfAlarmEventFreebusyJournalTodo, public NCAL::UnionOfEventFreebusyJournalTodo, public NCAL::UnionOfAlarmEventFreebusyTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo, public NCAL::UnionOfEventFreebusy, public NCAL::UnionOfEventJournalTodo, public NCAL::UnionOfEventTodo, public NCAL::UnionOfAlarmEventTodo
{
public:
    Event(Nepomuk::SimpleResource* res)
      : NCAL::UnionOfEventJournalTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo(res), NCAL::UnionOfAlarmEventJournalTodo(res), NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo(res), NCAL::UnionOfAlarmEventFreebusyJournalTodo(res), NCAL::UnionOfEventFreebusyJournalTodo(res), NCAL::UnionOfAlarmEventFreebusyTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo(res), NCAL::UnionOfEventFreebusy(res), NCAL::UnionOfEventJournalTodo(res), NCAL::UnionOfEventTodo(res), NCAL::UnionOfAlarmEventTodo(res), m_res(res)
    {}

    virtual ~Event() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#eventStatus. 
     * Defines the overall status or confirmation for an Event. Based 
     * on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    QUrl eventStatus() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#eventStatus", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#eventStatus", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#eventStatus. 
     * Defines the overall status or confirmation for an Event. Based 
     * on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    void setEventStatus(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#eventStatus", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#eventStatus. 
     * Defines the overall status or confirmation for an Event. Based 
     * on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    void addEventStatus(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#eventStatus", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#transp. 
     * Defines whether an event is transparent or not to busy time searches. 
     * Inspired by RFC 2445 sec.4.8.2.7. Values for this property 
     * can be chosen from a limited vocabulary. To express this a TimeTransparency 
     * class has been introduced. 
     */
    QUrl transp() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#transp", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#transp", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#transp. 
     * Defines whether an event is transparent or not to busy time searches. 
     * Inspired by RFC 2445 sec.4.8.2.7. Values for this property 
     * can be chosen from a limited vocabulary. To express this a TimeTransparency 
     * class has been introduced. 
     */
    void setTransp(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#transp", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#transp. 
     * Defines whether an event is transparent or not to busy time searches. 
     * Inspired by RFC 2445 sec.4.8.2.7. Values for this property 
     * can be chosen from a limited vocabulary. To express this a TimeTransparency 
     * class has been introduced. 
     */
    void addTransp(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#transp", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Event", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
