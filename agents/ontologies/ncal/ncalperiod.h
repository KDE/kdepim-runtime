#ifndef _NCAL_NCALPERIOD_H_
#define _NCAL_NCALPERIOD_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "ncal/ncaltimeentity.h"
namespace Nepomuk {
namespace NCAL {
/**
 * A period of time. Inspired by the PERIOD datatype specified 
 * in RFC 2445 sec. 4.3.9 
 */
class NcalPeriod : public NCAL::NcalTimeEntity
{
public:
    NcalPeriod(Nepomuk::SimpleResource* res)
      : NCAL::NcalTimeEntity(res), m_res(res)
    {}

    virtual ~NcalPeriod() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodBegin. 
     * Beginng of a period. Inspired by the first part of a structured 
     * value of the PERIOD datatype specified in RFC 2445 sec. 4.3.9 
     */
    QDateTime periodBegin() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodBegin", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodBegin", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodBegin. 
     * Beginng of a period. Inspired by the first part of a structured 
     * value of the PERIOD datatype specified in RFC 2445 sec. 4.3.9 
     */
    void setPeriodBegin(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodBegin", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodBegin. 
     * Beginng of a period. Inspired by the first part of a structured 
     * value of the PERIOD datatype specified in RFC 2445 sec. 4.3.9 
     */
    void addPeriodBegin(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodBegin", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodEnd. 
     * End of a period of time. Inspired by the second part of a structured 
     * value of a PERIOD datatype specified in RFC 2445 sec. 4.3.9. 
     * Note that a single NcalPeriod instance shouldn't have the periodEnd 
     * and periodDuration properties specified simultaneously. 
     */
    QDateTime periodEnd() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodEnd", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodEnd", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodEnd. 
     * End of a period of time. Inspired by the second part of a structured 
     * value of a PERIOD datatype specified in RFC 2445 sec. 4.3.9. 
     * Note that a single NcalPeriod instance shouldn't have the periodEnd 
     * and periodDuration properties specified simultaneously. 
     */
    void setPeriodEnd(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodEnd", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodEnd. 
     * End of a period of time. Inspired by the second part of a structured 
     * value of a PERIOD datatype specified in RFC 2445 sec. 4.3.9. 
     * Note that a single NcalPeriod instance shouldn't have the periodEnd 
     * and periodDuration properties specified simultaneously. 
     */
    void addPeriodEnd(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodEnd", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodDuration. 
     * Duration of a period of time. Inspired by the second part of a 
     * structured value of the PERIOD datatype specified in RFC 2445 
     * sec. 4.3.9. Note that a single NcalPeriod instance shouldn't 
     * have the periodEnd and periodDuration properties specified 
     * simultaneously. 
     */
    QUrl periodDuration() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodDuration", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodDuration", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodDuration. 
     * Duration of a period of time. Inspired by the second part of a 
     * structured value of the PERIOD datatype specified in RFC 2445 
     * sec. 4.3.9. Note that a single NcalPeriod instance shouldn't 
     * have the periodEnd and periodDuration properties specified 
     * simultaneously. 
     */
    void setPeriodDuration(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodDuration", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodDuration. 
     * Duration of a period of time. Inspired by the second part of a 
     * structured value of the PERIOD datatype specified in RFC 2445 
     * sec. 4.3.9. Note that a single NcalPeriod instance shouldn't 
     * have the periodEnd and periodDuration properties specified 
     * simultaneously. 
     */
    void addPeriodDuration(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#periodDuration", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#NcalPeriod", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
