#ifndef _NCAL_EVENTSTATUS_H_
#define _NCAL_EVENTSTATUS_H_

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
 * A status of an event. This class has been introduced to express 
 * the limited set of values for the ncal:status property. The 
 * user may use the instances provided with this ontology or create 
 * his/her own. See the documentation for ncal:eventStatus for 
 * details. 
 */
class EventStatus
{
public:
    EventStatus(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~EventStatus() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#EventStatus", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
