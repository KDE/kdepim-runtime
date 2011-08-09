#ifndef _NCO_PCSNUMBER_H_
#define _NCO_PCSNUMBER_H_

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
 * Personal Communication Services Number. A class inspired 
 * by the TYPE=pcs parameter of the TEL property defined in RFC 
 * 2426 sec. 3.3.1 
 */
class PcsNumber : public NCO::VoicePhoneNumber
{
public:
    PcsNumber(Nepomuk::SimpleResource* res)
      : NCO::VoicePhoneNumber(res), m_res(res)
    {}

    virtual ~PcsNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#PcsNumber", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
