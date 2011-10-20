#ifndef _NMO_MESSAGE_H_
#define _NMO_MESSAGE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

namespace Nepomuk {
namespace NMO {
/**
 * A message. Could be an email, instant messanging message, SMS 
 * message etc. 
 */
class Message
{
public:
    Message(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Message() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#replyTo. 
     * deprecated in favor of nmo:messageReplyTo 
     */
    QList<QUrl> replyTos() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#replyTo", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#replyTo. 
     * deprecated in favor of nmo:messageReplyTo 
     */
    void setReplyTos(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#replyTo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#replyTo. 
     * deprecated in favor of nmo:messageReplyTo 
     */
    void addReplyTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#replyTo", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#plainTextMessageContent. 
     * Plain text representation of the body of the message. For multipart 
     * messages, all parts are concatenated into the value of this 
     * property. Attachments, whose mimeTypes are different from 
     * text/plain or message/rfc822 are considered separate DataObjects 
     * and are therefore not included in the value of this property. 
     */
    QStringList plainTextMessageContents() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#plainTextMessageContent", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#plainTextMessageContent. 
     * Plain text representation of the body of the message. For multipart 
     * messages, all parts are concatenated into the value of this 
     * property. Attachments, whose mimeTypes are different from 
     * text/plain or message/rfc822 are considered separate DataObjects 
     * and are therefore not included in the value of this property. 
     */
    void setPlainTextMessageContents(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#plainTextMessageContent", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#plainTextMessageContent. 
     * Plain text representation of the body of the message. For multipart 
     * messages, all parts are concatenated into the value of this 
     * property. Attachments, whose mimeTypes are different from 
     * text/plain or message/rfc822 are considered separate DataObjects 
     * and are therefore not included in the value of this property. 
     */
    void addPlainTextMessageContent(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#plainTextMessageContent", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryRecipient. 
     * deprecated in favor of nmo:secondaryMessageRecipient 
     */
    QList<QUrl> secondaryRecipients() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryRecipient", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryRecipient. 
     * deprecated in favor of nmo:secondaryMessageRecipient 
     */
    void setSecondaryRecipients(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryRecipient", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryRecipient. 
     * deprecated in favor of nmo:secondaryMessageRecipient 
     */
    void addSecondaryRecipient(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryRecipient", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#hasAttachment. 
     * Links a message with files that were sent as attachments. 
     */
    QList<QUrl> hasAttachments() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#hasAttachment", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#hasAttachment. 
     * Links a message with files that were sent as attachments. 
     */
    void setHasAttachments(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#hasAttachment", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#hasAttachment. 
     * Links a message with files that were sent as attachments. 
     */
    void addHasAttachment(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#hasAttachment", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryMessageRecipient. 
     * A superproperty for all "additional" recipients of a message. 
     */
    QList<QUrl> secondaryMessageRecipients() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryMessageRecipient", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryMessageRecipient. 
     * A superproperty for all "additional" recipients of a message. 
     */
    void setSecondaryMessageRecipients(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryMessageRecipient", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryMessageRecipient. 
     * A superproperty for all "additional" recipients of a message. 
     */
    void addSecondaryMessageRecipient(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#secondaryMessageRecipient", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageId. 
     * An identifier of a message. This property has been inspired 
     * by the message-id property defined in RFC 2822, Sec. 3.6.4. 
     * It should be used for all kinds of identifiers used by various 
     * messaging applications to connect multiple messages into 
     * conversations. For email messageids, values are according 
     * to RFC2822/sec 3.6.4 and the literal value in RDF must include 
     * the brackets. 
     */
    QStringList messageIds() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageId", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageId. 
     * An identifier of a message. This property has been inspired 
     * by the message-id property defined in RFC 2822, Sec. 3.6.4. 
     * It should be used for all kinds of identifiers used by various 
     * messaging applications to connect multiple messages into 
     * conversations. For email messageids, values are according 
     * to RFC2822/sec 3.6.4 and the literal value in RDF must include 
     * the brackets. 
     */
    void setMessageIds(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageId", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageId. 
     * An identifier of a message. This property has been inspired 
     * by the message-id property defined in RFC 2822, Sec. 3.6.4. 
     * It should be used for all kinds of identifiers used by various 
     * messaging applications to connect multiple messages into 
     * conversations. For email messageids, values are according 
     * to RFC2822/sec 3.6.4 and the literal value in RDF must include 
     * the brackets. 
     */
    void addMessageId(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageId", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageFrom. 
     * The sender of the message 
     */
    QUrl messageFrom() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageFrom", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageFrom", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageFrom. 
     * The sender of the message 
     */
    void setMessageFrom(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageFrom", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageFrom. 
     * The sender of the message 
     */
    void addMessageFrom(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageFrom", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSender. 
     * The person or agent submitting the message to the network, if 
     * other from the one given with the nmo:from property. Defined 
     * in RFC 822 sec. 4.4.2 
     */
    QUrl messageSender() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSender", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSender", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSender. 
     * The person or agent submitting the message to the network, if 
     * other from the one given with the nmo:from property. Defined 
     * in RFC 822 sec. 4.4.2 
     */
    void setMessageSender(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSender", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSender. 
     * The person or agent submitting the message to the network, if 
     * other from the one given with the nmo:from property. Defined 
     * in RFC 822 sec. 4.4.2 
     */
    void addMessageSender(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSender", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#inReplyTo. 
     * Signifies that a message is a reply to another message. This 
     * feature is commonly used to link messages into conversations. 
     * Note that it is more specific than nmo:references. See RFC 2822 
     * sec. 3.6.4 
     */
    QUrl inReplyTo() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#inReplyTo", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#inReplyTo", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#inReplyTo. 
     * Signifies that a message is a reply to another message. This 
     * feature is commonly used to link messages into conversations. 
     * Note that it is more specific than nmo:references. See RFC 2822 
     * sec. 3.6.4 
     */
    void setInReplyTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#inReplyTo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#inReplyTo. 
     * Signifies that a message is a reply to another message. This 
     * feature is commonly used to link messages into conversations. 
     * Note that it is more specific than nmo:references. See RFC 2822 
     * sec. 3.6.4 
     */
    void addInReplyTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#inReplyTo", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#from. 
     * deprecated in favor of nmo:messageFrom 
     */
    QList<QUrl> froms() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#from", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#from. 
     * deprecated in favor of nmo:messageFrom 
     */
    void setFroms(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#from", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#from. 
     * deprecated in favor of nmo:messageFrom 
     */
    void addFrom(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#from", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sender. 
     * deprecated in favor of nmo:messageSender 
     */
    QList<QUrl> senders() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sender", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sender. 
     * deprecated in favor of nmo:messageSender 
     */
    void setSenders(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sender", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sender. 
     * deprecated in favor of nmo:messageSender 
     */
    void addSender(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sender", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#htmlMessageContent. 
     * HTML representation of the body of the message. For multipart 
     * messages, all parts are concatenated into the value of this 
     * property. Attachments, whose mimeTypes are different from 
     * text/plain or message/rfc822 are considered separate DataObjects 
     * and are therefore not included in the value of this property. 
     */
    QStringList htmlMessageContents() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#htmlMessageContent", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#htmlMessageContent. 
     * HTML representation of the body of the message. For multipart 
     * messages, all parts are concatenated into the value of this 
     * property. Attachments, whose mimeTypes are different from 
     * text/plain or message/rfc822 are considered separate DataObjects 
     * and are therefore not included in the value of this property. 
     */
    void setHtmlMessageContents(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#htmlMessageContent", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#htmlMessageContent. 
     * HTML representation of the body of the message. For multipart 
     * messages, all parts are concatenated into the value of this 
     * property. Attachments, whose mimeTypes are different from 
     * text/plain or message/rfc822 are considered separate DataObjects 
     * and are therefore not included in the value of this property. 
     */
    void addHtmlMessageContent(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#htmlMessageContent", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryMessageRecipient. 
     * The primary intended recipient of a message. 
     */
    QList<QUrl> primaryMessageRecipients() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryMessageRecipient", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryMessageRecipient. 
     * The primary intended recipient of a message. 
     */
    void setPrimaryMessageRecipients(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryMessageRecipient", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryMessageRecipient. 
     * The primary intended recipient of a message. 
     */
    void addPrimaryMessageRecipient(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryMessageRecipient", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sentDate. 
     * Date when this message was sent. 
     */
    QDateTime sentDate() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sentDate", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sentDate", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sentDate. 
     * Date when this message was sent. 
     */
    void setSentDate(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sentDate", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sentDate. 
     * Date when this message was sent. 
     */
    void addSentDate(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#sentDate", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#recipient. 
     * deprecated in favor of nmo:messageRecipient 
     */
    QList<QUrl> recipients() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#recipient", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#recipient. 
     * deprecated in favor of nmo:messageRecipient 
     */
    void setRecipients(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#recipient", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#recipient. 
     * deprecated in favor of nmo:messageRecipient 
     */
    void addRecipient(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#recipient", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSubject. 
     * The subject of a message 
     */
    QString messageSubject() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSubject", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSubject", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSubject. 
     * The subject of a message 
     */
    void setMessageSubject(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSubject", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSubject. 
     * The subject of a message 
     */
    void addMessageSubject(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageSubject", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#references. 
     * Signifies that a message references another message. This 
     * property is a generic one. See RFC 2822 Sec. 3.6.4 
     */
    QList<QUrl> referenceses() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#references", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#references. 
     * Signifies that a message references another message. This 
     * property is a generic one. See RFC 2822 Sec. 3.6.4 
     */
    void setReferenceses(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#references", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#references. 
     * Signifies that a message references another message. This 
     * property is a generic one. See RFC 2822 Sec. 3.6.4 
     */
    void addReferences(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#references", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageRecipient. 
     * A common superproperty for all properties that link a message 
     * with its recipients. Please don't use this property directly. 
     */
    QList<QUrl> messageRecipients() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageRecipient", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageRecipient. 
     * A common superproperty for all properties that link a message 
     * with its recipients. Please don't use this property directly. 
     */
    void setMessageRecipients(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageRecipient", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageRecipient. 
     * A common superproperty for all properties that link a message 
     * with its recipients. Please don't use this property directly. 
     */
    void addMessageRecipient(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageRecipient", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#receivedDate. 
     * Date when this message was received. 
     */
    QDateTime receivedDate() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#receivedDate", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#receivedDate", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#receivedDate. 
     * Date when this message was received. 
     */
    void setReceivedDate(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#receivedDate", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#receivedDate. 
     * Date when this message was received. 
     */
    void addReceivedDate(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#receivedDate", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#isRead. 
     * A flag that states the fact that a MailboxDataObject has been 
     * read. 
     */
    bool isRead() const {
        bool value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#isRead", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#isRead", QUrl::StrictMode)).first().value<bool>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#isRead. 
     * A flag that states the fact that a MailboxDataObject has been 
     * read. 
     */
    void setIsRead(const bool& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#isRead", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#isRead. 
     * A flag that states the fact that a MailboxDataObject has been 
     * read. 
     */
    void addIsRead(const bool& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#isRead", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageHeader. 
     * Links the message wiith an arbitrary message header. 
     */
    QUrl messageHeader() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageHeader", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageHeader", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageHeader. 
     * Links the message wiith an arbitrary message header. 
     */
    void setMessageHeader(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageHeader", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageHeader. 
     * Links the message wiith an arbitrary message header. 
     */
    void addMessageHeader(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageHeader", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryRecipient. 
     * deprecated in favor of primaryMessageRecipient 
     */
    QList<QUrl> primaryRecipients() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryRecipient", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryRecipient. 
     * deprecated in favor of primaryMessageRecipient 
     */
    void setPrimaryRecipients(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryRecipient", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryRecipient. 
     * deprecated in favor of primaryMessageRecipient 
     */
    void addPrimaryRecipient(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#primaryRecipient", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageReplyTo. 
     * An address where the reply should be sent. 
     */
    QUrl messageReplyTo() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageReplyTo", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageReplyTo", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageReplyTo. 
     * An address where the reply should be sent. 
     */
    void setMessageReplyTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageReplyTo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageReplyTo. 
     * An address where the reply should be sent. 
     */
    void addMessageReplyTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#messageReplyTo", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#Message", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
