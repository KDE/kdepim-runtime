#ifndef _NCO_AUDIOIMACCOUNT_H_
#define _NCO_AUDIOIMACCOUNT_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nco/imaccount.h"
namespace Nepomuk {
namespace NCO {
/**
 * Deprecated in favour of nco:imCapabilityAudio. 
 */
class AudioIMAccount : public NCO::IMAccount
{
public:
    AudioIMAccount(Nepomuk::SimpleResource* res)
      : NCO::IMAccount(res), m_res(res)
    {}

    virtual ~AudioIMAccount() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#AudioIMAccount", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
