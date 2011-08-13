#ifndef _NCO_CELLPHONENUMBER_H_
#define _NCO_CELLPHONENUMBER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nco/voicephonenumber.h"
#include "nco/messagingnumber.h"
namespace Nepomuk {
namespace NCO {
/**
 * A cellular phone number. Inspired by the (TYPE=cell) parameter 
 * of the TEL property as defined in RFC 2426 sec 3.3.1. Usually 
 * a cellular phone can accept voice calls as well as textual messages 
 * (SMS), therefore this class has two superclasses. 
 */
class CellPhoneNumber : public NCO::VoicePhoneNumber, public NCO::MessagingNumber
{
public:
    CellPhoneNumber(Nepomuk::SimpleResource* res)
      : NCO::VoicePhoneNumber(res), NCO::MessagingNumber(res), m_res(res)
    {}

    virtual ~CellPhoneNumber() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#CellPhoneNumber", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
