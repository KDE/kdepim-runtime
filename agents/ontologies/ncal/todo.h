#ifndef _NCAL_TODO_H_
#define _NCAL_TODO_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "ncal/unionofeventjournaltimezonetodo.h"
#include "ncal/unionofalarmeventjournaltodo.h"
#include "ncal/unionofeventfreebusyjournaltodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusytimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusyjournaltimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventjournaltimezonetodo.h"
#include "ncal/unionofalarmeventtodo.h"
#include "ncal/unionofeventjournaltodo.h"
#include "ncal/unionofeventtodo.h"
#include "ncal/unionofalarmeventfreebusyjournaltodo.h"
#include "ncal/unionofalarmeventfreebusytodo.h"
namespace Nepomuk {
namespace NCAL {
/**
 * Provide a grouping of calendar properties that describe a to-do. 
 */
class Todo : public NCAL::UnionOfEventJournalTimezoneTodo, public NCAL::UnionOfAlarmEventJournalTodo, public NCAL::UnionOfEventFreebusyJournalTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo, public NCAL::UnionOfAlarmEventTodo, public NCAL::UnionOfEventJournalTodo, public NCAL::UnionOfEventTodo, public NCAL::UnionOfAlarmEventFreebusyJournalTodo, public NCAL::UnionOfAlarmEventFreebusyTodo
{
public:
    Todo(Nepomuk::SimpleResource* res)
      : NCAL::UnionOfEventJournalTimezoneTodo(res), NCAL::UnionOfAlarmEventJournalTodo(res), NCAL::UnionOfEventFreebusyJournalTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo(res), NCAL::UnionOfAlarmEventTodo(res), NCAL::UnionOfEventJournalTodo(res), NCAL::UnionOfEventTodo(res), NCAL::UnionOfAlarmEventFreebusyJournalTodo(res), NCAL::UnionOfAlarmEventFreebusyTodo(res), m_res(res)
    {}

    virtual ~Todo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#percentComplete. 
     * This property is used by an assignee or delegatee of a to-do to 
     * convey the percent completion of a to-do to the Organizer. Inspired 
     * by RFC 2445 sec. 4.8.1.8 
     */
    QList<qint64> percentCompletes() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#percentComplete", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#percentComplete. 
     * This property is used by an assignee or delegatee of a to-do to 
     * convey the percent completion of a to-do to the Organizer. Inspired 
     * by RFC 2445 sec. 4.8.1.8 
     */
    void setPercentCompletes(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#percentComplete", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#percentComplete. 
     * This property is used by an assignee or delegatee of a to-do to 
     * convey the percent completion of a to-do to the Organizer. Inspired 
     * by RFC 2445 sec. 4.8.1.8 
     */
    void addPercentComplete(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#percentComplete", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#completed. 
     * This property defines the date and time that a to-do was actually 
     * completed. Inspired by RFC 2445 sec. 4.8.2.1. Note that the 
     * RFC allows ONLY UTC time values for this property. 
     */
    QDateTime completed() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#completed", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#completed", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#completed. 
     * This property defines the date and time that a to-do was actually 
     * completed. Inspired by RFC 2445 sec. 4.8.2.1. Note that the 
     * RFC allows ONLY UTC time values for this property. 
     */
    void setCompleted(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#completed", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#completed. 
     * This property defines the date and time that a to-do was actually 
     * completed. Inspired by RFC 2445 sec. 4.8.2.1. Note that the 
     * RFC allows ONLY UTC time values for this property. 
     */
    void addCompleted(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#completed", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#due. 
     * This property defines the date and time that a to-do is expected 
     * to be completed. Inspired by RFC 2445 sec. 4.8.2.3 
     */
    QUrl due() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#due", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#due", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#due. 
     * This property defines the date and time that a to-do is expected 
     * to be completed. Inspired by RFC 2445 sec. 4.8.2.3 
     */
    void setDue(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#due", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#due. 
     * This property defines the date and time that a to-do is expected 
     * to be completed. Inspired by RFC 2445 sec. 4.8.2.3 
     */
    void addDue(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#due", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#todoStatus. 
     * Defines the overall status or confirmation for a todo. Based 
     * on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    QUrl todoStatus() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#todoStatus", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#todoStatus", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#todoStatus. 
     * Defines the overall status or confirmation for a todo. Based 
     * on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    void setTodoStatus(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#todoStatus", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#todoStatus. 
     * Defines the overall status or confirmation for a todo. Based 
     * on the STATUS property defined in RFC 2445 sec. 4.8.1.11. 
     */
    void addTodoStatus(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#todoStatus", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Todo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
