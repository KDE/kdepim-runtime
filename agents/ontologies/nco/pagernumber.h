#ifndef _NCO_PAGERNUMBER_H_
#define _NCO_PAGERNUMBER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

#include "nco/messagingnumber.h"
namespace Nepomuk2 {
namespace NCO {
/**
 * A pager phone number. Inspired by the (TYPE=pager) parameter 
 * of the TEL property as defined in RFC 2426 sec 3.3.1. 
 */
class PagerNumber : public NCO::MessagingNumber
{
public:
    PagerNumber(Nepomuk2::SimpleResource* res)
      : NCO::MessagingNumber(res), m_res(res)
    {}

    virtual ~PagerNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#PagerNumber", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
