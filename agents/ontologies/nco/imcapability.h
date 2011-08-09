#ifndef _NCO_IMCAPABILITY_H_
#define _NCO_IMCAPABILITY_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

namespace Nepomuk {
namespace NCO {
/**
 * Capabilities of a cetain IMAccount. 
 */
class IMCapability
{
public:
    IMCapability(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~IMCapability() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#IMCapability", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
