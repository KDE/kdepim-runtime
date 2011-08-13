#ifndef _NCAL_ATTACHMENT_H_
#define _NCAL_ATTACHMENT_H_

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
 * An object attached to a calendar entity. This class has been 
 * introduced to serve as a structured value of the ncal:attach 
 * property. See the documentation of ncal:attach for details. 
 */
class Attachment
{
public:
    Attachment(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Attachment() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentUri. 
     * The uri of the attachment. Created to express the actual value 
     * of the ATTACH property defined in RFC 2445 sec. 4.8.1.1. This 
     * property expresses the default URI datatype of that property. 
     * see ncal:attachmentContents for the BINARY datatype. 
     */
    QUrl attachmentUri() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentUri", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentUri", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentUri. 
     * The uri of the attachment. Created to express the actual value 
     * of the ATTACH property defined in RFC 2445 sec. 4.8.1.1. This 
     * property expresses the default URI datatype of that property. 
     * see ncal:attachmentContents for the BINARY datatype. 
     */
    void setAttachmentUri(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentUri", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentUri. 
     * The uri of the attachment. Created to express the actual value 
     * of the ATTACH property defined in RFC 2445 sec. 4.8.1.1. This 
     * property expresses the default URI datatype of that property. 
     * see ncal:attachmentContents for the BINARY datatype. 
     */
    void addAttachmentUri(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentUri", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fmttype. 
     * To specify the content type of a referenced object. Inspired 
     * by RFC 2445 sec. 4.2.8. The value of this property should be an 
     * IANA-registered content type (e.g. application/binary) 
     */
    QString fmttype() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fmttype", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fmttype", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fmttype. 
     * To specify the content type of a referenced object. Inspired 
     * by RFC 2445 sec. 4.2.8. The value of this property should be an 
     * IANA-registered content type (e.g. application/binary) 
     */
    void setFmttype(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fmttype", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fmttype. 
     * To specify the content type of a referenced object. Inspired 
     * by RFC 2445 sec. 4.2.8. The value of this property should be an 
     * IANA-registered content type (e.g. application/binary) 
     */
    void addFmttype(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#fmttype", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentContent. 
     * The uri of the attachment. Created to express the actual value 
     * of the ATTACH property defined in RFC 2445 sec. 4.8.1.1. This 
     * property expresses the BINARY datatype of that property. see 
     * ncal:attachmentUri for the URI datatype. 
     */
    QStringList attachmentContents() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentContent", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentContent. 
     * The uri of the attachment. Created to express the actual value 
     * of the ATTACH property defined in RFC 2445 sec. 4.8.1.1. This 
     * property expresses the BINARY datatype of that property. see 
     * ncal:attachmentUri for the URI datatype. 
     */
    void setAttachmentContents(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentContent", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentContent. 
     * The uri of the attachment. Created to express the actual value 
     * of the ATTACH property defined in RFC 2445 sec. 4.8.1.1. This 
     * property expresses the BINARY datatype of that property. see 
     * ncal:attachmentUri for the URI datatype. 
     */
    void addAttachmentContent(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attachmentContent", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#encoding. 
     * To specify an alternate inline encoding for the property value. 
     * Inspired by RFC 2445 sec. 4.2.7. Originally this property had 
     * a limited vocabulary. ('8BIT' and 'BASE64'). The terms of this 
     * vocabulary have been expressed as instances of the AttachmentEncoding 
     * class 
     */
    QUrl encoding() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#encoding", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#encoding", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#encoding. 
     * To specify an alternate inline encoding for the property value. 
     * Inspired by RFC 2445 sec. 4.2.7. Originally this property had 
     * a limited vocabulary. ('8BIT' and 'BASE64'). The terms of this 
     * vocabulary have been expressed as instances of the AttachmentEncoding 
     * class 
     */
    void setEncoding(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#encoding", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#encoding. 
     * To specify an alternate inline encoding for the property value. 
     * Inspired by RFC 2445 sec. 4.2.7. Originally this property had 
     * a limited vocabulary. ('8BIT' and 'BASE64'). The terms of this 
     * vocabulary have been expressed as instances of the AttachmentEncoding 
     * class 
     */
    void addEncoding(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#encoding", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Attachment", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
