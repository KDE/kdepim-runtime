#ifndef _NCO_CONTACTLISTDATAOBJECT_H_
#define _NCO_CONTACTLISTDATAOBJECT_H_

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
 * An entity occuring on a contact list (usually interpreted as 
 * an nco:Contact) 
 */
class ContactListDataObject
{
public:
    ContactListDataObject(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~ContactListDataObject() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#ContactListDataObject", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
