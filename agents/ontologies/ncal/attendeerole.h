#ifndef _NCAL_ATTENDEEROLE_H_
#define _NCAL_ATTENDEEROLE_H_

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
 * A role the attendee is going to play during an event. This class 
 * has been introduced to express the limited vocabulary for the 
 * values of ncal:role property. Please refer to the documentation 
 * of ncal:role for details. 
 */
class AttendeeRole
{
public:
    AttendeeRole(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~AttendeeRole() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#AttendeeRole", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
