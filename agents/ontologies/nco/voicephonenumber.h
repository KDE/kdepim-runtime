#ifndef _NCO_VOICEPHONENUMBER_H_
#define _NCO_VOICEPHONENUMBER_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/phonenumber.h"
namespace Nepomuk {
namespace NCO {
/**
 * A telephone number with voice communication capabilities. 
 * Class inspired by the TYPE=voice parameter of the TEL property 
 * defined in RFC 2426 sec. 3.3.1 
 */
class VoicePhoneNumber : public NCO::PhoneNumber
{
public:
    VoicePhoneNumber(Nepomuk::SimpleResource* res)
      : NCO::PhoneNumber(res), m_res(res)
    {}

    virtual ~VoicePhoneNumber() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#voiceMail. 
     * Indicates if the given number accepts voice mail. (e.g. there 
     * is an answering machine). Inspired by TYPE=msg parameter of 
     * the TEL property defined in RFC 2426 sec. 3.3.1 
     */
    bool voiceMail() const {
        bool value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#voiceMail", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#voiceMail", QUrl::StrictMode)).first().value<bool>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#voiceMail. 
     * Indicates if the given number accepts voice mail. (e.g. there 
     * is an answering machine). Inspired by TYPE=msg parameter of 
     * the TEL property defined in RFC 2426 sec. 3.3.1 
     */
    void setVoiceMail(const bool& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#voiceMail", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#voiceMail. 
     * Indicates if the given number accepts voice mail. (e.g. there 
     * is an answering machine). Inspired by TYPE=msg parameter of 
     * the TEL property defined in RFC 2426 sec. 3.3.1 
     */
    void addVoiceMail(const bool& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#voiceMail", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#VoicePhoneNumber", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
