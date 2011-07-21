#ifndef _NCAL_UNIONOFEVENTTODO_H_
#define _NCAL_UNIONOFEVENTTODO_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk/simpleresource.h>

#include "ncal/unionparentclass.h"
namespace Nepomuk {
namespace NCAL {
/**
 * 
 */
class UnionOfEventTodo : public NCAL::UnionParentClass
{
public:
    UnionOfEventTodo(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfEventTodo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#hasAlarm. 
     * Links an event or a todo with a DataObject that can be interpreted 
     * as an alarm. This property has no direct equivalent in the RFC 
     * 2445. It has been provided to express this relation. 
     */
    QList<QUrl> hasAlarms() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#hasAlarm", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#hasAlarm. 
     * Links an event or a todo with a DataObject that can be interpreted 
     * as an alarm. This property has no direct equivalent in the RFC 
     * 2445. It has been provided to express this relation. 
     */
    void setHasAlarms(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#hasAlarm", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#hasAlarm. 
     * Links an event or a todo with a DataObject that can be interpreted 
     * as an alarm. This property has no direct equivalent in the RFC 
     * 2445. It has been provided to express this relation. 
     */
    void addHasAlarm(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#hasAlarm", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#priority. 
     * The property defines the relative priority for a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.9 
     */
    qint64 priority() const {
        qint64 value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#priority", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#priority", QUrl::StrictMode)).first().value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#priority. 
     * The property defines the relative priority for a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.9 
     */
    void setPriority(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#priority", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#priority. 
     * The property defines the relative priority for a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.9 
     */
    void addPriority(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#priority", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#geo. 
     * This property specifies information related to the global 
     * position for the activity specified by a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.6 
     */
    QUrl geo() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#geo", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#geo", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#geo. 
     * This property specifies information related to the global 
     * position for the activity specified by a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.6 
     */
    void setGeo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#geo", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#geo. 
     * This property specifies information related to the global 
     * position for the activity specified by a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.6 
     */
    void addGeo(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#geo", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#locationAltRep. 
     * Alternate representation of the event or todo location. Introduced 
     * to cover the ALTREP parameter of the LOCATION property. See 
     * documentation of ncal:location for details. 
     */
    QList<QUrl> locationAltReps() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#locationAltRep", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#locationAltRep. 
     * Alternate representation of the event or todo location. Introduced 
     * to cover the ALTREP parameter of the LOCATION property. See 
     * documentation of ncal:location for details. 
     */
    void setLocationAltReps(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#locationAltRep", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#locationAltRep. 
     * Alternate representation of the event or todo location. Introduced 
     * to cover the ALTREP parameter of the LOCATION property. See 
     * documentation of ncal:location for details. 
     */
    void addLocationAltRep(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#locationAltRep", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resourcesAltRep. 
     * Alternate representation of the resources needed for an event 
     * or todo. Introduced to cover the ALTREP parameter of the resources 
     * property. See documentation for ncal:resources for details. 
     */
    QList<QUrl> resourcesAltReps() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resourcesAltRep", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resourcesAltRep. 
     * Alternate representation of the resources needed for an event 
     * or todo. Introduced to cover the ALTREP parameter of the resources 
     * property. See documentation for ncal:resources for details. 
     */
    void setResourcesAltReps(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resourcesAltRep", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resourcesAltRep. 
     * Alternate representation of the resources needed for an event 
     * or todo. Introduced to cover the ALTREP parameter of the resources 
     * property. See documentation for ncal:resources for details. 
     */
    void addResourcesAltRep(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resourcesAltRep", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resources. 
     * Defines the equipment or resources anticipated for an activity 
     * specified by a calendar entity. Inspired by RFC 2445 sec. 4.8.1.10 
     * with the following reservations: the LANGUAGE parameter has 
     * been discarded. Please use xml:lang literals to express language. 
     * For the ALTREP parameter use the resourcesAltRep property. 
     * This property specifies multiple resources. The order is not 
     * important. it is recommended to introduce a separate triple 
     * for each resource. 
     */
    QStringList resourceses() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resources", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resources. 
     * Defines the equipment or resources anticipated for an activity 
     * specified by a calendar entity. Inspired by RFC 2445 sec. 4.8.1.10 
     * with the following reservations: the LANGUAGE parameter has 
     * been discarded. Please use xml:lang literals to express language. 
     * For the ALTREP parameter use the resourcesAltRep property. 
     * This property specifies multiple resources. The order is not 
     * important. it is recommended to introduce a separate triple 
     * for each resource. 
     */
    void setResourceses(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resources", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resources. 
     * Defines the equipment or resources anticipated for an activity 
     * specified by a calendar entity. Inspired by RFC 2445 sec. 4.8.1.10 
     * with the following reservations: the LANGUAGE parameter has 
     * been discarded. Please use xml:lang literals to express language. 
     * For the ALTREP parameter use the resourcesAltRep property. 
     * This property specifies multiple resources. The order is not 
     * important. it is recommended to introduce a separate triple 
     * for each resource. 
     */
    void addResources(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#resources", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#location. 
     * Defines the intended venue for the activity defined by a calendar 
     * component. Inspired by RFC 2445 sec 4.8.1.7 with the following 
     * reservations: the LANGUAGE parameter has been discarded. 
     * Please use xml:lang literals to express language. For the ALTREP 
     * parameter use the locationAltRep property. 
     */
    QString location() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#location", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#location", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#location. 
     * Defines the intended venue for the activity defined by a calendar 
     * component. Inspired by RFC 2445 sec 4.8.1.7 with the following 
     * reservations: the LANGUAGE parameter has been discarded. 
     * Please use xml:lang literals to express language. For the ALTREP 
     * parameter use the locationAltRep property. 
     */
    void setLocation(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#location", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#location. 
     * Defines the intended venue for the activity defined by a calendar 
     * component. Inspired by RFC 2445 sec 4.8.1.7 with the following 
     * reservations: the LANGUAGE parameter has been discarded. 
     * Please use xml:lang literals to express language. For the ALTREP 
     * parameter use the locationAltRep property. 
     */
    void addLocation(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#location", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfEventTodo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
