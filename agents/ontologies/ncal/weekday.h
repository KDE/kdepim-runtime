#ifndef _NCAL_WEEKDAY_H_
#define _NCAL_WEEKDAY_H_

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
 * Day of the week. This class has been created to provide the limited 
 * vocabulary for ncal:byday property. See the documentation 
 * for ncal:byday for details. 
 */
class Weekday
{
public:
    Weekday(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Weekday() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Weekday", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
