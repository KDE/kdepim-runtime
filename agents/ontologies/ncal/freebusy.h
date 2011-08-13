#ifndef _NCAL_FREEBUSY_H_
#define _NCAL_FREEBUSY_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "ncal/unionofeventfreebusyjournaltodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusytimezonetodo.h"
#include "ncal/unionofeventfreebusy.h"
#include "ncal/unionofalarmeventfreebusytodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusyjournaltimezonetodo.h"
#include "ncal/unionofalarmeventfreebusyjournaltodo.h"
namespace Nepomuk {
namespace NCAL {
/**
 * Provide a grouping of component properties that describe either 
 * a request for free/busy time, describe a response to a request 
 * for free/busy time or describe a published set of busy time. 
 */
class Freebusy : public NCAL::UnionOfEventFreebusyJournalTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo, public NCAL::UnionOfEventFreebusy, public NCAL::UnionOfAlarmEventFreebusyTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo, public NCAL::UnionOfAlarmEventFreebusyJournalTodo
{
public:
    Freebusy(Nepomuk::SimpleResource* res)
      : NCAL::UnionOfEventFreebusyJournalTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo(res), NCAL::UnionOfEventFreebusy(res), NCAL::UnionOfAlarmEventFreebusyTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo(res), NCAL::UnionOfAlarmEventFreebusyJournalTodo(res), m_res(res)
    {}

    virtual ~Freebusy() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freebusy. 
     * The property defines one or more free or busy time intervals. 
     * Inspired by RFC 2445 sec. 4.8.2.6. Note that the periods specified 
     * by this property can only be expressed with UTC times. Originally 
     * this property could have many comma-separated values. Please 
     * use a separate triple for each value. 
     */
    QList<QUrl> freebusys() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freebusy", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freebusy. 
     * The property defines one or more free or busy time intervals. 
     * Inspired by RFC 2445 sec. 4.8.2.6. Note that the periods specified 
     * by this property can only be expressed with UTC times. Originally 
     * this property could have many comma-separated values. Please 
     * use a separate triple for each value. 
     */
    void setFreebusys(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freebusy", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freebusy. 
     * The property defines one or more free or busy time intervals. 
     * Inspired by RFC 2445 sec. 4.8.2.6. Note that the periods specified 
     * by this property can only be expressed with UTC times. Originally 
     * this property could have many comma-separated values. Please 
     * use a separate triple for each value. 
     */
    void addFreebusy(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freebusy", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Freebusy", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
