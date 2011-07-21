#ifndef _NCAL_CALENDARUSERTYPE_H_
#define _NCAL_CALENDARUSERTYPE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

namespace Nepomuk {
namespace NCAL {
/**
 * A calendar user type. This class has been introduced to express 
 * the limited vocabulary for the ncal:cutype property. See documentation 
 * of ncal:cutype for details. 
 */
class CalendarUserType
{
public:
    CalendarUserType(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~CalendarUserType() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#CalendarUserType", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
