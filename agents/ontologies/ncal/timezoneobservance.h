#ifndef _NCAL_TIMEZONEOBSERVANCE_H_
#define _NCAL_TIMEZONEOBSERVANCE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "ncal/unionoftimezoneobservanceeventfreebusytimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventfreebusyjournaltimezonetodo.h"
#include "ncal/unionoftimezoneobservanceeventjournaltimezonetodo.h"
namespace Nepomuk {
namespace NCAL {
/**
 * 
 */
class TimezoneObservance : public NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo, public NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo
{
public:
    TimezoneObservance(Nepomuk::SimpleResource* res)
      : NCAL::UnionOfTimezoneObservanceEventFreebusyTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo(res), NCAL::UnionOfTimezoneObservanceEventJournalTimezoneTodo(res), m_res(res)
    {}

    virtual ~TimezoneObservance() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetto. 
     * This property specifies the offset which is in use in this time 
     * zone observance. nspired by RFC 2445 sec. 4.8.3.4. The original 
     * domain was underspecified. It said that this property must 
     * appear within a Timezone component. In this ontology a TimezoneObservance 
     * class has been introduced to clarify this specification. The 
     * original range was UTC-OFFSET. There is no equivalent among 
     * the XSD datatypes so plain string was chosen. 
     */
    QString tzoffsetto() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetto", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetto", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetto. 
     * This property specifies the offset which is in use in this time 
     * zone observance. nspired by RFC 2445 sec. 4.8.3.4. The original 
     * domain was underspecified. It said that this property must 
     * appear within a Timezone component. In this ontology a TimezoneObservance 
     * class has been introduced to clarify this specification. The 
     * original range was UTC-OFFSET. There is no equivalent among 
     * the XSD datatypes so plain string was chosen. 
     */
    void setTzoffsetto(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetto", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetto. 
     * This property specifies the offset which is in use in this time 
     * zone observance. nspired by RFC 2445 sec. 4.8.3.4. The original 
     * domain was underspecified. It said that this property must 
     * appear within a Timezone component. In this ontology a TimezoneObservance 
     * class has been introduced to clarify this specification. The 
     * original range was UTC-OFFSET. There is no equivalent among 
     * the XSD datatypes so plain string was chosen. 
     */
    void addTzoffsetto(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetto", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetfrom. 
     * This property specifies the offset which is in use prior to this 
     * time zone observance. Inspired by RFC 2445 sec. 4.8.3.3. The 
     * original domain was underspecified. It said that this property 
     * must appear within a Timezone component. In this ontology a 
     * TimezoneObservance class has been introduced to clarify this 
     * specification. The original range was UTC-OFFSET. There is 
     * no equivalent among the XSD datatypes so plain string was chosen. 
     */
    QString tzoffsetfrom() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetfrom", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetfrom", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetfrom. 
     * This property specifies the offset which is in use prior to this 
     * time zone observance. Inspired by RFC 2445 sec. 4.8.3.3. The 
     * original domain was underspecified. It said that this property 
     * must appear within a Timezone component. In this ontology a 
     * TimezoneObservance class has been introduced to clarify this 
     * specification. The original range was UTC-OFFSET. There is 
     * no equivalent among the XSD datatypes so plain string was chosen. 
     */
    void setTzoffsetfrom(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetfrom", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetfrom. 
     * This property specifies the offset which is in use prior to this 
     * time zone observance. Inspired by RFC 2445 sec. 4.8.3.3. The 
     * original domain was underspecified. It said that this property 
     * must appear within a Timezone component. In this ontology a 
     * TimezoneObservance class has been introduced to clarify this 
     * specification. The original range was UTC-OFFSET. There is 
     * no equivalent among the XSD datatypes so plain string was chosen. 
     */
    void addTzoffsetfrom(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzoffsetfrom", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzname. 
     * Specifies the customary designation for a timezone description. 
     * Inspired by RFC 2445 sec. 4.8.3.2 The LANGUAGE parameter has 
     * been discarded. Please xml:lang literals to express languages. 
     * Original specification for the domain of this property stated 
     * that it must appear within the timezone component. In this ontology 
     * the TimezoneObservance class has been itroduced to clarify 
     * this specification. 
     */
    QStringList tznames() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzname", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzname. 
     * Specifies the customary designation for a timezone description. 
     * Inspired by RFC 2445 sec. 4.8.3.2 The LANGUAGE parameter has 
     * been discarded. Please xml:lang literals to express languages. 
     * Original specification for the domain of this property stated 
     * that it must appear within the timezone component. In this ontology 
     * the TimezoneObservance class has been itroduced to clarify 
     * this specification. 
     */
    void setTznames(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzname", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzname. 
     * Specifies the customary designation for a timezone description. 
     * Inspired by RFC 2445 sec. 4.8.3.2 The LANGUAGE parameter has 
     * been discarded. Please xml:lang literals to express languages. 
     * Original specification for the domain of this property stated 
     * that it must appear within the timezone component. In this ontology 
     * the TimezoneObservance class has been itroduced to clarify 
     * this specification. 
     */
    void addTzname(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#tzname", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#TimezoneObservance", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
