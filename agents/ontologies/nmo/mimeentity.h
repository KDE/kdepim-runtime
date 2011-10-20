#ifndef _NMO_MIMEENTITY_H_
#define _NMO_MIMEENTITY_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

namespace Nepomuk {
namespace NMO {
/**
 * A MIME entity, as defined in RFC2045, Section 2.4. 
 */
class MimeEntity
{
public:
    MimeEntity(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~MimeEntity() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#MimeEntity", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
