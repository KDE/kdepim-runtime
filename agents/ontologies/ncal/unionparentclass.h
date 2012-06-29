#ifndef _NCAL_UNIONPARENTCLASS_H_
#define _NCAL_UNIONPARENTCLASS_H_

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
 * 
 */
class UnionParentClass
{
public:
    UnionParentClass(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~UnionParentClass() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionParentClass", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
