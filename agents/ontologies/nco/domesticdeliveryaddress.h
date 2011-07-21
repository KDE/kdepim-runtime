#ifndef _NCO_DOMESTICDELIVERYADDRESS_H_
#define _NCO_DOMESTICDELIVERYADDRESS_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/postaladdress.h"
namespace Nepomuk {
namespace NCO {
/**
 * Domestic Delivery Addresse. Class inspired by TYPE=dom parameter 
 * of the ADR property defined in RFC 2426 sec. 3.2.1 
 */
class DomesticDeliveryAddress : public NCO::PostalAddress
{
public:
    DomesticDeliveryAddress(Nepomuk::SimpleResource* res)
      : NCO::PostalAddress(res), m_res(res)
    {}

    virtual ~DomesticDeliveryAddress() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#DomesticDeliveryAddress", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
