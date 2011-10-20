#ifndef _NMO_IMMESSAGE_H_
#define _NMO_IMMESSAGE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nmo/message.h"
namespace Nepomuk {
namespace NMO {
/**
 * A message sent with Instant Messaging software. 
 */
class IMMessage : public NMO::Message
{
public:
    IMMessage(Nepomuk::SimpleResource* res)
      : NMO::Message(res), m_res(res)
    {}

    virtual ~IMMessage() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#IMMessage", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
