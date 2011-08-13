#ifndef _NCAL_TRIGGER_H_
#define _NCAL_TRIGGER_H_

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
 * An alarm trigger. This class has been created to serve as the 
 * range of ncal:trigger property. See the documentation for 
 * ncal:trigger for more details. 
 */
class Trigger
{
public:
    Trigger(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Trigger() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDuration. 
     * The duration of a trigger. This property has been created to 
     * express the VALUE=DURATION parameter of the TRIGGER property. 
     * See documentation for ncal:trigger for more details. 
     */
    QUrl triggerDuration() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDuration", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDuration", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDuration. 
     * The duration of a trigger. This property has been created to 
     * express the VALUE=DURATION parameter of the TRIGGER property. 
     * See documentation for ncal:trigger for more details. 
     */
    void setTriggerDuration(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDuration", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDuration. 
     * The duration of a trigger. This property has been created to 
     * express the VALUE=DURATION parameter of the TRIGGER property. 
     * See documentation for ncal:trigger for more details. 
     */
    void addTriggerDuration(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDuration", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDateTime. 
     * The exact date and time of the trigger. This property has been 
     * created to express the VALUE=DATE, and VALUE=DATE-TIME parameters 
     * of the TRIGGER property. See the documentation for ncal:trigger 
     * for more details 
     */
    QDateTime triggerDateTime() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDateTime", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDateTime", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDateTime. 
     * The exact date and time of the trigger. This property has been 
     * created to express the VALUE=DATE, and VALUE=DATE-TIME parameters 
     * of the TRIGGER property. See the documentation for ncal:trigger 
     * for more details 
     */
    void setTriggerDateTime(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDateTime", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDateTime. 
     * The exact date and time of the trigger. This property has been 
     * created to express the VALUE=DATE, and VALUE=DATE-TIME parameters 
     * of the TRIGGER property. See the documentation for ncal:trigger 
     * for more details 
     */
    void addTriggerDateTime(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#triggerDateTime", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#related. 
     * To specify the relationship of the alarm trigger with respect 
     * to the start or end of the calendar component. Inspired by RFC 
     * 2445 4.2.14. The RFC has specified two possible values for this 
     * property ('START' and 'END') they have been expressed as instances 
     * of the TriggerRelation class. 
     */
    QList<QUrl> relateds() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#related", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#related. 
     * To specify the relationship of the alarm trigger with respect 
     * to the start or end of the calendar component. Inspired by RFC 
     * 2445 4.2.14. The RFC has specified two possible values for this 
     * property ('START' and 'END') they have been expressed as instances 
     * of the TriggerRelation class. 
     */
    void setRelateds(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#related", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#related. 
     * To specify the relationship of the alarm trigger with respect 
     * to the start or end of the calendar component. Inspired by RFC 
     * 2445 4.2.14. The RFC has specified two possible values for this 
     * property ('START' and 'END') they have been expressed as instances 
     * of the TriggerRelation class. 
     */
    void addRelated(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#related", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Trigger", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
