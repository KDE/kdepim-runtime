#ifndef _NMO_MAILBOX_H_
#define _NMO_MAILBOX_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

namespace Nepomuk2 {
namespace NMO {
/**
 * A mailbox - container for MailboxDataObjects. 
 */
class Mailbox
{
public:
    Mailbox(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Mailbox() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#Mailbox", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
