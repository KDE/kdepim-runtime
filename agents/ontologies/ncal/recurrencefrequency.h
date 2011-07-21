#ifndef _NCAL_RECURRENCEFREQUENCY_H_
#define _NCAL_RECURRENCEFREQUENCY_H_

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
 * Frequency of a recurrence rule. This class has been introduced 
 * to express a limited set of allowed values for the ncal:freq 
 * property. See the documentation of ncal:freq for details. 
 */
class RecurrenceFrequency
{
public:
    RecurrenceFrequency(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~RecurrenceFrequency() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#RecurrenceFrequency", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
