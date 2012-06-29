#ifndef _NCAL_RECURRENCEFREQUENCY_H_
#define _NCAL_RECURRENCEFREQUENCY_H_

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
 * Frequency of a recurrence rule. This class has been introduced 
 * to express a limited set of allowed values for the ncal:freq 
 * property. See the documentation of ncal:freq for details. 
 */
class RecurrenceFrequency
{
public:
    RecurrenceFrequency(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~RecurrenceFrequency() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#RecurrenceFrequency", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
