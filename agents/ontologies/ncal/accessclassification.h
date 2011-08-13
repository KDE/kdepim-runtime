#ifndef _NCAL_ACCESSCLASSIFICATION_H_
#define _NCAL_ACCESSCLASSIFICATION_H_

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
 * Access classification of a calendar component. Introduced 
 * to express the set of values for the ncal:class property. The 
 * user may use instances provided with this ontology or create 
 * his/her own with desired semantics. See the documentation 
 * of ncal:class for details. 
 */
class AccessClassification
{
public:
    AccessClassification(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~AccessClassification() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#AccessClassification", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
