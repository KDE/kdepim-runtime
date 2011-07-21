#ifndef _NCAL_TRIGGERRELATION_H_
#define _NCAL_TRIGGERRELATION_H_

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
 * The relation between the trigger and its parent calendar component. 
 * This class has been introduced to express the limited vocabulary 
 * for the ncal:related property. See the documentation for ncal:related 
 * for more details. 
 */
class TriggerRelation
{
public:
    TriggerRelation(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~TriggerRelation() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#TriggerRelation", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
