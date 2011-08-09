#ifndef _NCAL_JOURNAL_H_
#define _NCAL_JOURNAL_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "ncal/unionoftimezoneobservanceeventfreebusyjournaltimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventjournaltimezonetodo.h"
#include "ncal/unionofalarmeventjournaltodo.h"
#include "ncal/unionofeventjournaltimezonetodo.h"
#include "ncal/unionofeventfreebusyjournaltodo.h"
#include "ncal/unionofalarmeventfreebusyjournaltodo.h"
#include "ncal/unionofeventjournaltodo.h"
namespace Nepomuk {
namespace NCAL {
/**
 * Provide a grouping of component properties that describe a 
 * journal entry. 
 */
class Journal : public NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo, public NCAL::UnionOfAlarmEventJournalTodo, public NCAL::UnionOfEventJournalTimezoneTodo, public NCAL::UnionOfEventFreebusyJournalTodo, public NCAL::UnionOfAlarmEventFreebusyJournalTodo, public NCAL::UnionOfEventJournalTodo
{
public:
    Journal(Nepomuk::SimpleResource* res)
      : NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo(res), NCAL::UnionOfAlarmEventJournalTodo(res), NCAL::UnionOfEventJournalTimezoneTodo(res), NCAL::UnionOfEventFreebusyJournalTodo(res), NCAL::UnionOfAlarmEventFreebusyJournalTodo(res), NCAL::UnionOfEventJournalTodo(res), m_res(res)
    {}

    virtual ~Journal() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#journalStatus. 
     * Defines the overall status or confirmation for a journal entry. 
     * Based on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    QUrl journalStatus() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#journalStatus", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#journalStatus", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#journalStatus. 
     * Defines the overall status or confirmation for a journal entry. 
     * Based on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    void setJournalStatus(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#journalStatus", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#journalStatus. 
     * Defines the overall status or confirmation for a journal entry. 
     * Based on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    void addJournalStatus(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#journalStatus", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Journal", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
