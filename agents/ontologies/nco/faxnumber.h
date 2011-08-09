#ifndef _NCO_FAXNUMBER_H_
#define _NCO_FAXNUMBER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/phonenumber.h"
namespace Nepomuk {
namespace NCO {
/**
 * A fax number. Inspired by the (TYPE=fax) parameter of the TEL 
 * property as defined in RFC 2426 sec 3.3.1. 
 */
class FaxNumber : public NCO::PhoneNumber
{
public:
    FaxNumber(Nepomuk::SimpleResource* res)
      : NCO::PhoneNumber(res), m_res(res)
    {}

    virtual ~FaxNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#FaxNumber", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
