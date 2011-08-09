#ifndef _NCAL_ALARMACTION_H_
#define _NCAL_ALARMACTION_H_

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
 * Action to be performed on alarm. This class has been introduced 
 * to express the limited set of values of the ncal:action property. 
 * Please refer to the documentation of ncal:action for details. 
 */
class AlarmAction
{
public:
    AlarmAction(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~AlarmAction() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#AlarmAction", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
