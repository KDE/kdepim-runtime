#ifndef _NMO_MAILBOXDATAOBJECT_H_
#define _NMO_MAILBOXDATAOBJECT_H_

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
 * An entity encountered in a mailbox. Most common interpretations 
 * for such an entity include Message or Folder 
 */
class MailboxDataObject
{
public:
    MailboxDataObject(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~MailboxDataObject() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#MailboxDataObject", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
