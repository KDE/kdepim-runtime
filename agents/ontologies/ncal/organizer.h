#ifndef _NCAL_ORGANIZER_H_
#define _NCAL_ORGANIZER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "ncal/attendeeororganizer.h"
namespace Nepomuk {
namespace NCAL {
/**
 * An organizer of an event. This class has been introduced to serve 
 * as a range of ncal:organizer property. See documentation of 
 * ncal:organizer for details. 
 */
class Organizer : public NCAL::AttendeeOrOrganizer
{
public:
    Organizer(Nepomuk::SimpleResource* res)
      : NCAL::AttendeeOrOrganizer(res), m_res(res)
    {}

    virtual ~Organizer() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Organizer", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
