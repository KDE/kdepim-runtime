#ifndef _NCAL_TODOSTATUS_H_
#define _NCAL_TODOSTATUS_H_

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
 * A status of a calendar entity. This class has been introduced 
 * to express the limited set of values for the ncal:status property. 
 * The user may use the instances provided with this ontology or 
 * create his/her own. See the documentation for ncal:todoStatus 
 * for details. 
 */
class TodoStatus
{
public:
    TodoStatus(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~TodoStatus() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#TodoStatus", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
