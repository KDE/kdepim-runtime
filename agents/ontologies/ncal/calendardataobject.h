#ifndef _NCAL_CALENDARDATAOBJECT_H_
#define _NCAL_CALENDARDATAOBJECT_H_

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
 * A DataObject found in a calendar. It is usually interpreted 
 * as one of the calendar entity types (e.g. Event, Journal, Todo 
 * etc.) 
 */
class CalendarDataObject
{
public:
    CalendarDataObject(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~CalendarDataObject() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#CalendarDataObject", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
