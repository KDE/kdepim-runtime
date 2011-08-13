#ifndef _NCAL_ALARM_H_
#define _NCAL_ALARM_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "ncal/unionofalarmeventtodo.h"
#include "ncal/unionofalarmeventfreebusytodo.h"
#include "ncal/unionofalarmeventfreebusyjournaltodo.h"
#include "ncal/unionofalarmeventjournaltodo.h"
namespace Nepomuk {
namespace NCAL {
/**
 * Provide a grouping of component properties that define an alarm. 
 */
class Alarm : public NCAL::UnionOfAlarmEventTodo, public NCAL::UnionOfAlarmEventFreebusyTodo, public NCAL::UnionOfAlarmEventFreebusyJournalTodo, public NCAL::UnionOfAlarmEventJournalTodo
{
public:
    Alarm(Nepomuk::SimpleResource* res)
      : NCAL::UnionOfAlarmEventTodo(res), NCAL::UnionOfAlarmEventFreebusyTodo(res), NCAL::UnionOfAlarmEventFreebusyJournalTodo(res), NCAL::UnionOfAlarmEventJournalTodo(res), m_res(res)
    {}

    virtual ~Alarm() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#action. 
     * This property defines the action to be invoked when an alarm 
     * is triggered. Inspired by RFC 2445 sec 4.8.6.1. Originally 
     * this property had a limited set of values. They are expressed 
     * as instances of the AlarmAction class. 
     */
    QList<QUrl> actions() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#action", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#action. 
     * This property defines the action to be invoked when an alarm 
     * is triggered. Inspired by RFC 2445 sec 4.8.6.1. Originally 
     * this property had a limited set of values. They are expressed 
     * as instances of the AlarmAction class. 
     */
    void setActions(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#action", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#action. 
     * This property defines the action to be invoked when an alarm 
     * is triggered. Inspired by RFC 2445 sec 4.8.6.1. Originally 
     * this property had a limited set of values. They are expressed 
     * as instances of the AlarmAction class. 
     */
    void addAction(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#action", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#repeat. 
     * This property defines the number of time the alarm should be 
     * repeated, after the initial trigger. Inspired by RFC 2445 sec. 
     * 4.8.6.2 
     */
    qint64 repeat() const {
        qint64 value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#repeat", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#repeat", QUrl::StrictMode)).first().value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#repeat. 
     * This property defines the number of time the alarm should be 
     * repeated, after the initial trigger. Inspired by RFC 2445 sec. 
     * 4.8.6.2 
     */
    void setRepeat(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#repeat", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#repeat. 
     * This property defines the number of time the alarm should be 
     * repeated, after the initial trigger. Inspired by RFC 2445 sec. 
     * 4.8.6.2 
     */
    void addRepeat(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#repeat", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Alarm", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
