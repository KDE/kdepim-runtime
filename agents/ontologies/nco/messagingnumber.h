#ifndef _NCO_MESSAGINGNUMBER_H_
#define _NCO_MESSAGINGNUMBER_H_

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
 * A number that can accept textual messages. 
 */
class MessagingNumber : public NCO::PhoneNumber
{
public:
    MessagingNumber(Nepomuk::SimpleResource* res)
      : NCO::PhoneNumber(res), m_res(res)
    {}

    virtual ~MessagingNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#MessagingNumber", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
