#ifndef _NCO_VIDEOIMACCOUNT_H_
#define _NCO_VIDEOIMACCOUNT_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

#include "nco/audioimaccount.h"
namespace Nepomuk2 {
namespace NCO {
/**
 * Deprecated in favour of nco:imCapabilityVideo. 
 */
class VideoIMAccount : public NCO::AudioIMAccount
{
public:
    VideoIMAccount(Nepomuk2::SimpleResource* res)
      : NCO::AudioIMAccount(res), m_res(res)
    {}

    virtual ~VideoIMAccount() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#VideoIMAccount", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
