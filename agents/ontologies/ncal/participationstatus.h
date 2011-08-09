#ifndef _NCAL_PARTICIPATIONSTATUS_H_
#define _NCAL_PARTICIPATIONSTATUS_H_

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
 * Participation Status. This class has been introduced to express 
 * the limited vocabulary of values for the ncal:partstat property. 
 * See the documentation of ncal:partstat for details. 
 */
class ParticipationStatus
{
public:
    ParticipationStatus(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~ParticipationStatus() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ParticipationStatus", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
