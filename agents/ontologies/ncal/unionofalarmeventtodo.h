#ifndef _NCAL_UNIONOFALARMEVENTTODO_H_
#define _NCAL_UNIONOFALARMEVENTTODO_H_

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
class UnionOfAlarmEventTodo : public NCAL::UnionParentClass
{
public:
    UnionOfAlarmEventTodo(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfAlarmEventTodo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#trigger. 
     * This property specifies when an alarm will trigger. Inspired 
     * by RFC 2445 sec. 4.8.6.3 Originally the value of this property 
     * could accept two types : duration and date-time. To express 
     * this fact a Trigger class has been introduced. It also has a related 
     * property to account for the RELATED parameter. 
     */
    QList<QUrl> triggers() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#trigger", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#trigger. 
     * This property specifies when an alarm will trigger. Inspired 
     * by RFC 2445 sec. 4.8.6.3 Originally the value of this property 
     * could accept two types : duration and date-time. To express 
     * this fact a Trigger class has been introduced. It also has a related 
     * property to account for the RELATED parameter. 
     */
    void setTriggers(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#trigger", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#trigger. 
     * This property specifies when an alarm will trigger. Inspired 
     * by RFC 2445 sec. 4.8.6.3 Originally the value of this property 
     * could accept two types : duration and date-time. To express 
     * this fact a Trigger class has been introduced. It also has a related 
     * property to account for the RELATED parameter. 
     */
    void addTrigger(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#trigger", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfAlarmEventTodo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
