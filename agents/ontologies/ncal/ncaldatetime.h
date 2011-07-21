#ifndef _NCAL_NCALDATETIME_H_
#define _NCAL_NCALDATETIME_H_

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
 * 
 */
class NcalDateTime : public NCAL::NcalTimeEntity
{
public:
    NcalDateTime(Nepomuk::SimpleResource* res)
      : NCAL::NcalTimeEntity(res), m_res(res)
    {}

    virtual ~NcalDateTime() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#date. 
     * Date an instance of NcalDateTime refers to. It was conceived 
     * to express values in DATE datatype specified in RFC 2445 4.3.4 
     */
    QDate date() const {
        QDate value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#date", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#date", QUrl::StrictMode)).first().value<QDate>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#date. 
     * Date an instance of NcalDateTime refers to. It was conceived 
     * to express values in DATE datatype specified in RFC 2445 4.3.4 
     */
    void setDate(const QDate& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#date", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#date. 
     * Date an instance of NcalDateTime refers to. It was conceived 
     * to express values in DATE datatype specified in RFC 2445 4.3.4 
     */
    void addDate(const QDate& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#date", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dateTime. 
     * Representation of a date an instance of NcalDateTime actually 
     * refers to. It's purpose is to express values in DATE-TIME datatype, 
     * as defined in RFC 2445 sec. 4.3.5 
     */
    QDateTime dateTime() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dateTime", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dateTime", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dateTime. 
     * Representation of a date an instance of NcalDateTime actually 
     * refers to. It's purpose is to express values in DATE-TIME datatype, 
     * as defined in RFC 2445 sec. 4.3.5 
     */
    void setDateTime(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dateTime", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dateTime. 
     * Representation of a date an instance of NcalDateTime actually 
     * refers to. It's purpose is to express values in DATE-TIME datatype, 
     * as defined in RFC 2445 sec. 4.3.5 
     */
    void addDateTime(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dateTime", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalTimezone. 
     * The timezone instance that should be used to interpret an NcalDateTime. 
     * The purpose of this property is similar to the TZID parameter 
     * specified in RFC 2445 sec. 4.2.19 
     */
    QUrl ncalTimezone() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalTimezone", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalTimezone", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalTimezone. 
     * The timezone instance that should be used to interpret an NcalDateTime. 
     * The purpose of this property is similar to the TZID parameter 
     * specified in RFC 2445 sec. 4.2.19 
     */
    void setNcalTimezone(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalTimezone", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalTimezone. 
     * The timezone instance that should be used to interpret an NcalDateTime. 
     * The purpose of this property is similar to the TZID parameter 
     * specified in RFC 2445 sec. 4.2.19 
     */
    void addNcalTimezone(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalTimezone", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#NcalDateTime", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
