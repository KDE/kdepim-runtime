#ifndef _NCO_ISDNNUMBER_H_
#define _NCO_ISDNNUMBER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

#include "nco/voicephonenumber.h"
namespace Nepomuk2 {
namespace NCO {
/**
 * An ISDN phone number. Inspired by the (TYPE=isdn) parameter 
 * of the TEL property as defined in RFC 2426 sec 3.3.1. 
 */
class IsdnNumber : public NCO::VoicePhoneNumber
{
public:
    IsdnNumber(Nepomuk2::SimpleResource* res)
      : NCO::VoicePhoneNumber(res), m_res(res)
    {}

    virtual ~IsdnNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#IsdnNumber", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
