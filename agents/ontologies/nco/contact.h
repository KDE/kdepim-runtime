#ifndef _NCO_CONTACT_H_
#define _NCO_CONTACT_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "nco/role.h"
namespace Nepomuk {
namespace NCO {
/**
 * A Contact. A piece of data that can provide means to identify 
 * or communicate with an entity. 
 */
class Contact : public NCO::Role
{
public:
    Contact(Nepomuk::SimpleResource* res)
      : NCO::Role(res), m_res(res)
    {}

    virtual ~Contact() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#representative. 
     * An object that represent an object represented by this Contact. 
     * Usually this property is used to link a Contact to an organization, 
     * to a contact to the representative of this organization the 
     * user directly interacts with. An equivalent for the 'AGENT' 
     * property defined in RFC 2426 Sec. 3.5.4 
     */
    QList<QUrl> representatives() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#representative", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#representative. 
     * An object that represent an object represented by this Contact. 
     * Usually this property is used to link a Contact to an organization, 
     * to a contact to the representative of this organization the 
     * user directly interacts with. An equivalent for the 'AGENT' 
     * property defined in RFC 2426 Sec. 3.5.4 
     */
    void setRepresentatives(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#representative", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#representative. 
     * An object that represent an object represented by this Contact. 
     * Usually this property is used to link a Contact to an organization, 
     * to a contact to the representative of this organization the 
     * user directly interacts with. An equivalent for the 'AGENT' 
     * property defined in RFC 2426 Sec. 3.5.4 
     */
    void addRepresentative(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#representative", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#sound. 
     * Sound clip attached to a Contact. The DataObject referred to 
     * by this property is usually interpreted as an nfo:Audio. Inspired 
     * by the SOUND property defined in RFC 2425 sec. 3.6.6. 
     */
    QList<QUrl> sounds() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#sound", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#sound. 
     * Sound clip attached to a Contact. The DataObject referred to 
     * by this property is usually interpreted as an nfo:Audio. Inspired 
     * by the SOUND property defined in RFC 2425 sec. 3.6.6. 
     */
    void setSounds(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#sound", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#sound. 
     * Sound clip attached to a Contact. The DataObject referred to 
     * by this property is usually interpreted as an nfo:Audio. Inspired 
     * by the SOUND property defined in RFC 2425 sec. 3.6.6. 
     */
    void addSound(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#sound", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#birthDate. 
     * Birth date of the object represented by this Contact. An equivalent 
     * of the 'BDAY' property as defined in RFC 2426 Sec. 3.1.5. 
     */
    QDate birthDate() const {
        QDate value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#birthDate", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#birthDate", QUrl::StrictMode)).first().value<QDate>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#birthDate. 
     * Birth date of the object represented by this Contact. An equivalent 
     * of the 'BDAY' property as defined in RFC 2426 Sec. 3.1.5. 
     */
    void setBirthDate(const QDate& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#birthDate", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#birthDate. 
     * Birth date of the object represented by this Contact. An equivalent 
     * of the 'BDAY' property as defined in RFC 2426 Sec. 3.1.5. 
     */
    void addBirthDate(const QDate& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#birthDate", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#photo. 
     * Photograph attached to a Contact. The DataObject referred 
     * to by this property is usually interpreted as an nfo:Image. 
     * Inspired by the PHOTO property defined in RFC 2426 sec. 3.1.4 
     */
    QList<QUrl> photos() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#photo", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#photo. 
     * Photograph attached to a Contact. The DataObject referred 
     * to by this property is usually interpreted as an nfo:Image. 
     * Inspired by the PHOTO property defined in RFC 2426 sec. 3.1.4 
     */
    void setPhotos(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#photo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#photo. 
     * Photograph attached to a Contact. The DataObject referred 
     * to by this property is usually interpreted as an nfo:Image. 
     * Inspired by the PHOTO property defined in RFC 2426 sec. 3.1.4 
     */
    void addPhoto(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#photo", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#key. 
     * An encryption key attached to a contact. Inspired by the KEY 
     * property defined in RFC 2426 sec. 3.7.2 
     */
    QList<QUrl> keys() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#key", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#key. 
     * An encryption key attached to a contact. Inspired by the KEY 
     * property defined in RFC 2426 sec. 3.7.2 
     */
    void setKeys(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#key", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#key. 
     * An encryption key attached to a contact. Inspired by the KEY 
     * property defined in RFC 2426 sec. 3.7.2 
     */
    void addKey(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#key", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasLocation. 
     * Geographical location of the contact. Inspired by the 'GEO' 
     * property specified in RFC 2426 Sec. 3.4.2 
     */
    QUrl hasLocation() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasLocation", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasLocation", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasLocation. 
     * Geographical location of the contact. Inspired by the 'GEO' 
     * property specified in RFC 2426 Sec. 3.4.2 
     */
    void setHasLocation(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasLocation", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasLocation. 
     * Geographical location of the contact. Inspired by the 'GEO' 
     * property specified in RFC 2426 Sec. 3.4.2 
     */
    void addHasLocation(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasLocation", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#belongsToGroup. 
     * Links a Contact with a ContactGroup it belongs to. 
     */
    QList<QUrl> belongsToGroups() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#belongsToGroup", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#belongsToGroup. 
     * Links a Contact with a ContactGroup it belongs to. 
     */
    void setBelongsToGroups(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#belongsToGroup", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#belongsToGroup. 
     * Links a Contact with a ContactGroup it belongs to. 
     */
    void addBelongsToGroup(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#belongsToGroup", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#note. 
     * A note about the object represented by this Contact. An equivalent 
     * for the 'NOTE' property defined in RFC 2426 Sec. 3.6.2 
     */
    QStringList notes() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#note", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#note. 
     * A note about the object represented by this Contact. An equivalent 
     * for the 'NOTE' property defined in RFC 2426 Sec. 3.6.2 
     */
    void setNotes(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#note", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#note. 
     * A note about the object represented by this Contact. An equivalent 
     * for the 'NOTE' property defined in RFC 2426 Sec. 3.6.2 
     */
    void addNote(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#note", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nickname. 
     * A nickname of the Object represented by this Contact. This is 
     * an equivalent of the 'NICKNAME' property as defined in RFC 2426 
     * Sec. 3.1.3. 
     */
    QStringList nicknames() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nickname", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nickname. 
     * A nickname of the Object represented by this Contact. This is 
     * an equivalent of the 'NICKNAME' property as defined in RFC 2426 
     * Sec. 3.1.3. 
     */
    void setNicknames(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nickname", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nickname. 
     * A nickname of the Object represented by this Contact. This is 
     * an equivalent of the 'NICKNAME' property as defined in RFC 2426 
     * Sec. 3.1.3. 
     */
    void addNickname(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#nickname", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname. 
     * To specify the formatted text corresponding to the name of the 
     * object the Contact represents. An equivalent of the FN property 
     * as defined in RFC 2426 Sec. 3.1.1. 
     */
    QString fullname() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname. 
     * To specify the formatted text corresponding to the name of the 
     * object the Contact represents. An equivalent of the FN property 
     * as defined in RFC 2426 Sec. 3.1.1. 
     */
    void setFullname(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname. 
     * To specify the formatted text corresponding to the name of the 
     * object the Contact represents. An equivalent of the FN property 
     * as defined in RFC 2426 Sec. 3.1.1. 
     */
    void addFullname(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactUID. 
     * A value that represents a globally unique identifier corresponding 
     * to the individual or resource associated with the Contact. 
     * An equivalent of the 'UID' property defined in RFC 2426 Sec. 
     * 3.6.7 
     */
    QString contactUID() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactUID", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactUID", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactUID. 
     * A value that represents a globally unique identifier corresponding 
     * to the individual or resource associated with the Contact. 
     * An equivalent of the 'UID' property defined in RFC 2426 Sec. 
     * 3.6.7 
     */
    void setContactUID(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactUID", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactUID. 
     * A value that represents a globally unique identifier corresponding 
     * to the individual or resource associated with the Contact. 
     * An equivalent of the 'UID' property defined in RFC 2426 Sec. 
     * 3.6.7 
     */
    void addContactUID(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#contactUID", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
