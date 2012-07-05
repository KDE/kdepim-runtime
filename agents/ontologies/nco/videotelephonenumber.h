#ifndef _NCO_VIDEOTELEPHONENUMBER_H_
#define _NCO_VIDEOTELEPHONENUMBER_H_

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
 * A Video telephone number. A class inspired by the TYPE=video 
 * parameter of the TEL property defined in RFC 2426 sec. 3.3.1 
 */
class VideoTelephoneNumber : public NCO::VoicePhoneNumber
{
public:
    VideoTelephoneNumber(Nepomuk2::SimpleResource* res)
      : NCO::VoicePhoneNumber(res), m_res(res)
    {}

    virtual ~VideoTelephoneNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#VideoTelephoneNumber", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
