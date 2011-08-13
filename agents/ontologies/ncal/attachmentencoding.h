#ifndef _NCAL_ATTACHMENTENCODING_H_
#define _NCAL_ATTACHMENTENCODING_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

namespace Nepomuk {
namespace NCAL {
/**
 * Attachment encoding. This class has been introduced to express 
 * the limited vocabulary of values for the ncal:encoding property. 
 * See the documentation of ncal:encoding for details. 
 */
class AttachmentEncoding
{
public:
    AttachmentEncoding(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~AttachmentEncoding() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#AttachmentEncoding", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
