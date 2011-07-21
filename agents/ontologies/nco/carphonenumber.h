#ifndef _NCO_CARPHONENUMBER_H_
#define _NCO_CARPHONENUMBER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/voicephonenumber.h"
namespace Nepomuk {
namespace NCO {
/**
 * A car phone number. Inspired by the (TYPE=car) parameter of 
 * the TEL property as defined in RFC 2426 sec 3.3.1. 
 */
class CarPhoneNumber : public NCO::VoicePhoneNumber
{
public:
    CarPhoneNumber(Nepomuk::SimpleResource* res)
      : NCO::VoicePhoneNumber(res), m_res(res)
    {}

    virtual ~CarPhoneNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#CarPhoneNumber", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
