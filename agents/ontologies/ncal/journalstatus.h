#ifndef _NCAL_JOURNALSTATUS_H_
#define _NCAL_JOURNALSTATUS_H_

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
 * A status of a journal entry. This class has been introduced to 
 * express the limited set of values for the ncal:status property. 
 * The user may use the instances provided with this ontology or 
 * create his/her own. See the documentation for ncal:journalStatus 
 * for details. 
 */
class JournalStatus
{
public:
    JournalStatus(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~JournalStatus() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#JournalStatus", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
