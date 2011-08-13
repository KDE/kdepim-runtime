#ifndef _NCO_BBSNUMBER_H_
#define _NCO_BBSNUMBER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nco/modemnumber.h"
namespace Nepomuk {
namespace NCO {
/**
 * A Bulletin Board System (BBS) phone number. Inspired by the 
 * (TYPE=bbsl) parameter of the TEL property as defined in RFC 
 * 2426 sec 3.3.1. 
 */
class BbsNumber : public NCO::ModemNumber
{
public:
    BbsNumber(Nepomuk::SimpleResource* res)
      : NCO::ModemNumber(res), m_res(res)
    {}

    virtual ~BbsNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#BbsNumber", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
