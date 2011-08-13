#ifndef _NCAL_CALENDARSCALE_H_
#define _NCAL_CALENDARSCALE_H_

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
 * A calendar scale. This class has been introduced to provide 
 * the limited vocabulary for the ncal:calscale property. 
 */
class CalendarScale
{
public:
    CalendarScale(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~CalendarScale() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#CalendarScale", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
