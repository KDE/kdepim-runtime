#ifndef _NCAL_CALENDAR_H_
#define _NCAL_CALENDAR_H_

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
 * A calendar. Inspirations for this class can be traced to the 
 * VCALENDAR component defined in RFC 2445 sec. 4.4, but it may 
 * just as well be used to represent any kind of Calendar. 
 */
class Calendar
{
public:
    Calendar(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Calendar() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#component. 
     * Links the Vcalendar instance with the calendar components. 
     * This property has no direct equivalent in the RFC specification. 
     * It has been introduced to express the containmnent relations. 
     */
    QList<QUrl> components() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#component", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#component. 
     * Links the Vcalendar instance with the calendar components. 
     * This property has no direct equivalent in the RFC specification. 
     * It has been introduced to express the containmnent relations. 
     */
    void setComponents(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#component", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#component. 
     * Links the Vcalendar instance with the calendar components. 
     * This property has no direct equivalent in the RFC specification. 
     * It has been introduced to express the containmnent relations. 
     */
    void addComponent(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#component", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#version. 
     * This property specifies the identifier corresponding to the 
     * highest version number or the minimum and maximum range of the 
     * iCalendar specification that is required in order to interpret 
     * the iCalendar object. Defined in RFC 2445 sec. 4.7.4 
     */
    QStringList versions() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#version", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#version. 
     * This property specifies the identifier corresponding to the 
     * highest version number or the minimum and maximum range of the 
     * iCalendar specification that is required in order to interpret 
     * the iCalendar object. Defined in RFC 2445 sec. 4.7.4 
     */
    void setVersions(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#version", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#version. 
     * This property specifies the identifier corresponding to the 
     * highest version number or the minimum and maximum range of the 
     * iCalendar specification that is required in order to interpret 
     * the iCalendar object. Defined in RFC 2445 sec. 4.7.4 
     */
    void addVersion(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#version", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#method. 
     * This property defines the iCalendar object method associated 
     * with the calendar object. Defined in RFC 2445 sec. 4.7.2 
     */
    QString method() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#method", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#method", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#method. 
     * This property defines the iCalendar object method associated 
     * with the calendar object. Defined in RFC 2445 sec. 4.7.2 
     */
    void setMethod(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#method", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#method. 
     * This property defines the iCalendar object method associated 
     * with the calendar object. Defined in RFC 2445 sec. 4.7.2 
     */
    void addMethod(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#method", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#calscale. 
     * This property defines the calendar scale used for the calendar 
     * information specified in the iCalendar object. Defined in 
     * RFC 2445 sec. 4.7.1 
     */
    QUrl calscale() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#calscale", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#calscale", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#calscale. 
     * This property defines the calendar scale used for the calendar 
     * information specified in the iCalendar object. Defined in 
     * RFC 2445 sec. 4.7.1 
     */
    void setCalscale(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#calscale", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#calscale. 
     * This property defines the calendar scale used for the calendar 
     * information specified in the iCalendar object. Defined in 
     * RFC 2445 sec. 4.7.1 
     */
    void addCalscale(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#calscale", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#prodid. 
     * This property specifies the identifier for the product that 
     * created the iCalendar object. Defined in RFC 2445 sec. 4.7.2 
     */
    QStringList prodids() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#prodid", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#prodid. 
     * This property specifies the identifier for the product that 
     * created the iCalendar object. Defined in RFC 2445 sec. 4.7.2 
     */
    void setProdids(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#prodid", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#prodid. 
     * This property specifies the identifier for the product that 
     * created the iCalendar object. Defined in RFC 2445 sec. 4.7.2 
     */
    void addProdid(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#prodid", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#Calendar", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
