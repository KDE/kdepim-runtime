#ifndef _NCAL_RECURRENCEIDENTIFIER_H_
#define _NCAL_RECURRENCEIDENTIFIER_H_

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
 * Recurrence Identifier. Introduced to provide a structure 
 * for the value of ncal:recurrenceId property. See the documentation 
 * of ncal:recurrenceId for details. 
 */
class RecurrenceIdentifier
{
public:
    RecurrenceIdentifier(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~RecurrenceIdentifier() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#range. 
     * To specify the effective range of recurrence instances from 
     * the instance specified by the recurrence identifier specified 
     * by the property. It is intended to express the RANGE parameter 
     * specified in RFC 2445 sec. 4.2.13. The set of possible values 
     * for this property is limited. See also the documentation for 
     * ncal:recurrenceId for more details. 
     */
    QUrl range() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#range", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#range", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#range. 
     * To specify the effective range of recurrence instances from 
     * the instance specified by the recurrence identifier specified 
     * by the property. It is intended to express the RANGE parameter 
     * specified in RFC 2445 sec. 4.2.13. The set of possible values 
     * for this property is limited. See also the documentation for 
     * ncal:recurrenceId for more details. 
     */
    void setRange(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#range", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#range. 
     * To specify the effective range of recurrence instances from 
     * the instance specified by the recurrence identifier specified 
     * by the property. It is intended to express the RANGE parameter 
     * specified in RFC 2445 sec. 4.2.13. The set of possible values 
     * for this property is limited. See also the documentation for 
     * ncal:recurrenceId for more details. 
     */
    void addRange(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#range", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceIdDateTime. 
     * The date and time of a recurrence identifier. Provided to express 
     * the actual value of the ncal:recurrenceId property. See documentation 
     * for ncal:recurrenceId for details. 
     */
    QUrl recurrenceIdDateTime() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceIdDateTime", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceIdDateTime", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceIdDateTime. 
     * The date and time of a recurrence identifier. Provided to express 
     * the actual value of the ncal:recurrenceId property. See documentation 
     * for ncal:recurrenceId for details. 
     */
    void setRecurrenceIdDateTime(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceIdDateTime", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceIdDateTime. 
     * The date and time of a recurrence identifier. Provided to express 
     * the actual value of the ncal:recurrenceId property. See documentation 
     * for ncal:recurrenceId for details. 
     */
    void addRecurrenceIdDateTime(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#recurrenceIdDateTime", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#RecurrenceIdentifier", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
