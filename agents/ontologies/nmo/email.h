#ifndef _NMO_EMAIL_H_
#define _NMO_EMAIL_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nmo/message.h"
namespace Nepomuk {
namespace NMO {
/**
 * An email. 
 */
class Email : public NMO::Message
{
public:
    Email(Nepomuk::SimpleResource* res)
      : NMO::Message(res), m_res(res)
    {}

    virtual ~Email() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to. 
     * deprecated in favor of nmo:emailTo 
     */
    QList<QUrl> tos() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to. 
     * deprecated in favor of nmo:emailTo 
     */
    void setTos(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to. 
     * deprecated in favor of nmo:emailTo 
     */
    void addTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#to", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#cc. 
     * deprecated in favor of nmo:emailCc 
     */
    QList<QUrl> ccs() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#cc", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#cc. 
     * deprecated in favor of nmo:emailCc 
     */
    void setCcs(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#cc", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#cc. 
     * deprecated in favor of nmo:emailCc 
     */
    void addCc(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#cc", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailBcc. 
     * A Contact that is to receive a bcc of the email. A Bcc (blind carbon 
     * copy) is a copy of an email message sent to a recipient whose email 
     * address does not appear in the message. 
     */
    QList<QUrl> emailBccs() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailBcc", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailBcc. 
     * A Contact that is to receive a bcc of the email. A Bcc (blind carbon 
     * copy) is a copy of an email message sent to a recipient whose email 
     * address does not appear in the message. 
     */
    void setEmailBccs(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailBcc", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailBcc. 
     * A Contact that is to receive a bcc of the email. A Bcc (blind carbon 
     * copy) is a copy of an email message sent to a recipient whose email 
     * address does not appear in the message. 
     */
    void addEmailBcc(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailBcc", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailTo. 
     * The primary intended recipient of an email. 
     */
    QList<QUrl> emailTos() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailTo", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailTo. 
     * The primary intended recipient of an email. 
     */
    void setEmailTos(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailTo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailTo. 
     * The primary intended recipient of an email. 
     */
    void addEmailTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailTo", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#contentMimeType. 
     * Key used to store the MIME type of the content of an object when 
     * it is different from the object's main MIME type. This value 
     * can be used, for example, to model an e-mail message whose mime 
     * type is"message/rfc822", but whose content has type "text/html". 
     * If not specified, the MIME type of the content defaults to the 
     * value specified by the 'mimeType' property. 
     */
    QString contentMimeType() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#contentMimeType", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#contentMimeType", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#contentMimeType. 
     * Key used to store the MIME type of the content of an object when 
     * it is different from the object's main MIME type. This value 
     * can be used, for example, to model an e-mail message whose mime 
     * type is"message/rfc822", but whose content has type "text/html". 
     * If not specified, the MIME type of the content defaults to the 
     * value specified by the 'mimeType' property. 
     */
    void setContentMimeType(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#contentMimeType", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#contentMimeType. 
     * Key used to store the MIME type of the content of an object when 
     * it is different from the object's main MIME type. This value 
     * can be used, for example, to model an e-mail message whose mime 
     * type is"message/rfc822", but whose content has type "text/html". 
     * If not specified, the MIME type of the content defaults to the 
     * value specified by the 'mimeType' property. 
     */
    void addContentMimeType(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#contentMimeType", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailCc. 
     * A Contact that is to receive a cc of the email. A cc (carbon copy) 
     * is a copy of an email message whose recipient appears on the recipient 
     * list, so that all other recipients are aware of it. 
     */
    QList<QUrl> emailCcs() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailCc", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailCc. 
     * A Contact that is to receive a cc of the email. A cc (carbon copy) 
     * is a copy of an email message whose recipient appears on the recipient 
     * list, so that all other recipients are aware of it. 
     */
    void setEmailCcs(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailCc", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailCc. 
     * A Contact that is to receive a cc of the email. A cc (carbon copy) 
     * is a copy of an email message whose recipient appears on the recipient 
     * list, so that all other recipients are aware of it. 
     */
    void addEmailCc(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#emailCc", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#bcc. 
     * deprecated in favor of nmo:emailBcc 
     */
    QList<QUrl> bccs() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#bcc", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#bcc. 
     * deprecated in favor of nmo:emailBcc 
     */
    void setBccs(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#bcc", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#bcc. 
     * deprecated in favor of nmo:emailBcc 
     */
    void addBcc(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#bcc", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#Email", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
