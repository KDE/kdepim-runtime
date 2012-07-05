#ifndef _NCAL_FREEBUSYTYPE_H_
#define _NCAL_FREEBUSYTYPE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

namespace Nepomuk2 {
namespace NCAL {
/**
 * Type of a Freebusy indication. This class has been introduced 
 * to serve as a limited set of values for the ncal:fbtype property. 
 * See the documentation of ncal:fbtype for details. 
 */
class FreebusyType
{
public:
    FreebusyType(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~FreebusyType() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#FreebusyType", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
