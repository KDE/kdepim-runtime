#ifndef _NMO_MAILBOX_H_
#define _NMO_MAILBOX_H_

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
 * A mailbox - container for MailboxDataObjects. 
 */
class Mailbox
{
public:
    Mailbox(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Mailbox() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#Mailbox", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
