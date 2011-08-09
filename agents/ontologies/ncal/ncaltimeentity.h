#ifndef _NCAL_NCALTIMEENTITY_H_
#define _NCAL_NCALTIMEENTITY_H_

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
 * A time entity. Conceived as a common superclass for NcalDateTime 
 * and NcalPeriod. According to RFC 2445 both DateTime and Period 
 * can be interpreted in different timezones. The first case is 
 * explored in many properties. The second case is theoretically 
 * possible in ncal:rdate property. Therefore the timezone properties 
 * have been defined at this level. 
 */
class NcalTimeEntity
{
public:
    NcalTimeEntity(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~NcalTimeEntity() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#NcalTimeEntity", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
