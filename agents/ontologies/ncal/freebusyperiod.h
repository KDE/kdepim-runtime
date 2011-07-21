#ifndef _NCAL_FREEBUSYPERIOD_H_
#define _NCAL_FREEBUSYPERIOD_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "ncal/ncalperiod.h"
namespace Nepomuk {
namespace NCAL {
/**
 * An aggregate of a period and a freebusy type. This class has been 
 * introduced to serve as a range of the ncal:freebusy property. 
 * See documentation for ncal:freebusy for details. Note that 
 * the specification of freebusy property states that the period 
 * is to be expressed using UTC time, so the timezone properties 
 * should NOT be used for instances of this class. 
 */
class FreebusyPeriod : public NCAL::NcalPeriod
{
public:
    FreebusyPeriod(Nepomuk::SimpleResource* res)
      : NCAL::NcalPeriod(res), m_res(res)
    {}

    virtual ~FreebusyPeriod() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fbtype. 
     * To specify the free or busy time type. Inspired by RFC 2445 sec. 
     * 4.2.9. The RFC specified a limited vocabulary for the values 
     * of this property. The terms of this vocabulary have been expressed 
     * as instances of the FreebusyType class. The user can use instances 
     * provided with this ontology or create his own. 
     */
    QUrl fbtype() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fbtype", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fbtype", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fbtype. 
     * To specify the free or busy time type. Inspired by RFC 2445 sec. 
     * 4.2.9. The RFC specified a limited vocabulary for the values 
     * of this property. The terms of this vocabulary have been expressed 
     * as instances of the FreebusyType class. The user can use instances 
     * provided with this ontology or create his own. 
     */
    void setFbtype(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fbtype", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fbtype. 
     * To specify the free or busy time type. Inspired by RFC 2445 sec. 
     * 4.2.9. The RFC specified a limited vocabulary for the values 
     * of this property. The terms of this vocabulary have been expressed 
     * as instances of the FreebusyType class. The user can use instances 
     * provided with this ontology or create his own. 
     */
    void addFbtype(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fbtype", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#FreebusyPeriod", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
