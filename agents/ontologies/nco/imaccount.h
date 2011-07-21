#ifndef _NCO_IMACCOUNT_H_
#define _NCO_IMACCOUNT_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/contactmedium.h"
namespace Nepomuk {
namespace NCO {
/**
 * An account in an Instant Messaging system. 
 */
class IMAccount : public NCO::ContactMedium
{
public:
    IMAccount(Nepomuk::SimpleResource* res)
      : NCO::ContactMedium(res), m_res(res)
    {}

    virtual ~IMAccount() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMCapability. 
     * Indicates that an IMAccount has a certain capability. 
     */
    QList<QUrl> hasIMCapabilitys() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMCapability", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMCapability. 
     * Indicates that an IMAccount has a certain capability. 
     */
    void setHasIMCapabilitys(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMCapability", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMCapability. 
     * Indicates that an IMAccount has a certain capability. 
     */
    void addHasIMCapability(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasIMCapability", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imID. 
     * Identifier of the IM account. Examples of such identifier might 
     * include ICQ UINs, Jabber IDs, Skype names etc. 
     */
    QStringList imIDs() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imID", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imID. 
     * Identifier of the IM account. Examples of such identifier might 
     * include ICQ UINs, Jabber IDs, Skype names etc. 
     */
    void setImIDs(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imID", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imID. 
     * Identifier of the IM account. Examples of such identifier might 
     * include ICQ UINs, Jabber IDs, Skype names etc. 
     */
    void addImID(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imID", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#requestedPresenceSubscriptionTo. 
     * Indicates that this IMAccount has requested a subscription 
     * to the presence information of the other IMAccount. 
     */
    QList<QUrl> requestedPresenceSubscriptionTos() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#requestedPresenceSubscriptionTo", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#requestedPresenceSubscriptionTo. 
     * Indicates that this IMAccount has requested a subscription 
     * to the presence information of the other IMAccount. 
     */
    void setRequestedPresenceSubscriptionTos(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#requestedPresenceSubscriptionTo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#requestedPresenceSubscriptionTo. 
     * Indicates that this IMAccount has requested a subscription 
     * to the presence information of the other IMAccount. 
     */
    void addRequestedPresenceSubscriptionTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#requestedPresenceSubscriptionTo", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isAccessedBy. 
     * Indicates the local IMAccount by which this IMAccount is accessed. 
     * This does not imply membership of a contact list. 
     */
    QList<QUrl> isAccessedBys() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isAccessedBy", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isAccessedBy. 
     * Indicates the local IMAccount by which this IMAccount is accessed. 
     * This does not imply membership of a contact list. 
     */
    void setIsAccessedBys(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isAccessedBy", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isAccessedBy. 
     * Indicates the local IMAccount by which this IMAccount is accessed. 
     * This does not imply membership of a contact list. 
     */
    void addIsAccessedBy(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isAccessedBy", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imNickname. 
     * A nickname attached to a particular IM Account. 
     */
    QStringList imNicknames() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imNickname", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imNickname. 
     * A nickname attached to a particular IM Account. 
     */
    void setImNicknames(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imNickname", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imNickname. 
     * A nickname attached to a particular IM Account. 
     */
    void addImNickname(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imNickname", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isBlocked. 
     * Indicates that this IMAccount has been blocked. 
     */
    bool isBlocked() const {
        bool value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isBlocked", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isBlocked", QUrl::StrictMode)).first().value<bool>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isBlocked. 
     * Indicates that this IMAccount has been blocked. 
     */
    void setIsBlocked(const bool& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isBlocked", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isBlocked. 
     * Indicates that this IMAccount has been blocked. 
     */
    void addIsBlocked(const bool& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#isBlocked", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatusMessage. 
     * A feature common in most IM systems. A message left by the user 
     * for all his/her contacts to see. 
     */
    QString imStatusMessage() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatusMessage", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatusMessage", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatusMessage. 
     * A feature common in most IM systems. A message left by the user 
     * for all his/her contacts to see. 
     */
    void setImStatusMessage(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatusMessage", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatusMessage. 
     * A feature common in most IM systems. A message left by the user 
     * for all his/her contacts to see. 
     */
    void addImStatusMessage(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatusMessage", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#publishesPresenceTo. 
     * Indicates that this IMAccount publishes its presence information 
     * to the other IMAccount. 
     */
    QList<QUrl> publishesPresenceTos() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#publishesPresenceTo", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#publishesPresenceTo. 
     * Indicates that this IMAccount publishes its presence information 
     * to the other IMAccount. 
     */
    void setPublishesPresenceTos(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#publishesPresenceTo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#publishesPresenceTo. 
     * Indicates that this IMAccount publishes its presence information 
     * to the other IMAccount. 
     */
    void addPublishesPresenceTo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#publishesPresenceTo", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatus. 
     * Current status of the given IM account. Values for this property 
     * may include 'Online', 'Offline', 'Do not disturb' etc. The 
     * exact choice of them is unspecified. 
     */
    QString imStatus() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatus", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatus", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatus. 
     * Current status of the given IM account. Values for this property 
     * may include 'Online', 'Offline', 'Do not disturb' etc. The 
     * exact choice of them is unspecified. 
     */
    void setImStatus(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatus", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatus. 
     * Current status of the given IM account. Values for this property 
     * may include 'Online', 'Offline', 'Do not disturb' etc. The 
     * exact choice of them is unspecified. 
     */
    void addImStatus(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imStatus", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imAccountType. 
     * Type of the IM account. This may be the name of the service that 
     * provides the IM functionality. Examples might include Jabber, 
     * ICQ, MSN etc 
     */
    QString imAccountType() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imAccountType", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imAccountType", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imAccountType. 
     * Type of the IM account. This may be the name of the service that 
     * provides the IM functionality. Examples might include Jabber, 
     * ICQ, MSN etc 
     */
    void setImAccountType(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imAccountType", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imAccountType. 
     * Type of the IM account. This may be the name of the service that 
     * provides the IM functionality. Examples might include Jabber, 
     * ICQ, MSN etc 
     */
    void addImAccountType(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#imAccountType", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#IMAccount", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
