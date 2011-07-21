#ifndef _NCAL_TIMETRANSPARENCY_H_
#define _NCAL_TIMETRANSPARENCY_H_

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
 * Time transparency. Introduced to provide a way to express the 
 * limited vocabulary for the values of ncal:transp property. 
 * See documentation of ncal:transp for details. 
 */
class TimeTransparency
{
public:
    TimeTransparency(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~TimeTransparency() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#TimeTransparency", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
