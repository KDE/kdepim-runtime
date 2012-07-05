#ifndef _NCO_PARCELDELIVERYADDRESS_H_
#define _NCO_PARCELDELIVERYADDRESS_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

#include "nco/postaladdress.h"
namespace Nepomuk2 {
namespace NCO {
/**
 * Parcel Delivery Addresse. Class inspired by TYPE=parcel parameter 
 * of the ADR property defined in RFC 2426 sec. 3.2.1 
 */
class ParcelDeliveryAddress : public NCO::PostalAddress
{
public:
    ParcelDeliveryAddress(Nepomuk2::SimpleResource* res)
      : NCO::PostalAddress(res), m_res(res)
    {}

    virtual ~ParcelDeliveryAddress() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#ParcelDeliveryAddress", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
