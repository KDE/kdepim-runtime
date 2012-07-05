#ifndef _NCO_GENDER_H_
#define _NCO_GENDER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

namespace Nepomuk2 {
namespace NCO {
/**
 * Gender. Instances of this class may include male and female. 
 */
class Gender
{
public:
    Gender(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Gender() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Gender", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
