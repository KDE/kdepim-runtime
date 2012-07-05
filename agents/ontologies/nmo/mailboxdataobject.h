#ifndef _NMO_MAILBOXDATAOBJECT_H_
#define _NMO_MAILBOXDATAOBJECT_H_

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
 * An entity encountered in a mailbox. Most common interpretations 
 * for such an entity include Message or Folder 
 */
class MailboxDataObject
{
public:
    MailboxDataObject(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~MailboxDataObject() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#MailboxDataObject", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
