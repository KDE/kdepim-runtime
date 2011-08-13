#ifndef _NCAL_TIMEZONE_H_
#define _NCAL_TIMEZONE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "ncal/unionoftimezoneobservanceeventjournaltimezonetodo.h"
#include "ncal/unionofeventjournaltimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusyjournaltimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusytimezonetodo.h"
namespace Nepomuk {
namespace NCAL {
/**
 * Provide a grouping of component properties that defines a time 
 * zone. 
 */
class Timezone : public NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo, public NCAL::UnionOfEventJournalTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo
{
public:
    Timezone(Nepomuk::SimpleResource* res)
      : NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo(res), NCAL::UnionOfEventJournalTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo(res), m_res(res)
    {}

    virtual ~Timezone() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzurl. 
     * The TZURL provides a means for a VTIMEZONE component to point 
     * to a network location that can be used to retrieve an up-to- date 
     * version of itself. Inspired by RFC 2445 sec. 4.8.3.5. Originally 
     * the range of this property had been specified as URI. 
     */
    QList<QUrl> tzurls() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzurl", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzurl. 
     * The TZURL provides a means for a VTIMEZONE component to point 
     * to a network location that can be used to retrieve an up-to- date 
     * version of itself. Inspired by RFC 2445 sec. 4.8.3.5. Originally 
     * the range of this property had been specified as URI. 
     */
    void setTzurls(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzurl", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzurl. 
     * The TZURL provides a means for a VTIMEZONE component to point 
     * to a network location that can be used to retrieve an up-to- date 
     * version of itself. Inspired by RFC 2445 sec. 4.8.3.5. Originally 
     * the range of this property had been specified as URI. 
     */
    void addTzurl(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzurl", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#daylight. 
     * Links a timezone with it's daylight observance. This property 
     * has no direct equivalent in the RFC 2445. It has been inspired 
     * by the structure of the Vtimezone component defined in sec.4.6.5 
     */
    QUrl daylight() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#daylight", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#daylight", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#daylight. 
     * Links a timezone with it's daylight observance. This property 
     * has no direct equivalent in the RFC 2445. It has been inspired 
     * by the structure of the Vtimezone component defined in sec.4.6.5 
     */
    void setDaylight(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#daylight", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#daylight. 
     * Links a timezone with it's daylight observance. This property 
     * has no direct equivalent in the RFC 2445. It has been inspired 
     * by the structure of the Vtimezone component defined in sec.4.6.5 
     */
    void addDaylight(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#daylight", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#standard. 
     * Links the timezone with the standard timezone observance. 
     * This property has no direct equivalent in the RFC 2445. It has 
     * been inspired by the structure of the Vtimezone component defined 
     * in sec.4.6.5 
     */
    QUrl standard() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#standard", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#standard", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#standard. 
     * Links the timezone with the standard timezone observance. 
     * This property has no direct equivalent in the RFC 2445. It has 
     * been inspired by the structure of the Vtimezone component defined 
     * in sec.4.6.5 
     */
    void setStandard(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#standard", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#standard. 
     * Links the timezone with the standard timezone observance. 
     * This property has no direct equivalent in the RFC 2445. It has 
     * been inspired by the structure of the Vtimezone component defined 
     * in sec.4.6.5 
     */
    void addStandard(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#standard", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzid. 
     * This property specifies the text value that uniquely identifies 
     * the "VTIMEZONE" calendar component. Inspired by RFC 2445 sec 
     * 4.8.3.1 
     */
    QString tzid() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzid", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzid", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzid. 
     * This property specifies the text value that uniquely identifies 
     * the "VTIMEZONE" calendar component. Inspired by RFC 2445 sec 
     * 4.8.3.1 
     */
    void setTzid(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzid", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzid. 
     * This property specifies the text value that uniquely identifies 
     * the "VTIMEZONE" calendar component. Inspired by RFC 2445 sec 
     * 4.8.3.1 
     */
    void addTzid(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzid", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Timezone", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
