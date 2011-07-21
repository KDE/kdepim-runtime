#ifndef _NCAL_UNIONOFEVENTFREEBUSY_H_
#define _NCAL_UNIONOFEVENTFREEBUSY_H_

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
class UnionOfEventFreebusy : public NCAL::UnionParentClass
{
public:
    UnionOfEventFreebusy(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfEventFreebusy() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dtend. 
     * This property specifies the date and time that a calendar component 
     * ends. Inspired by RFC 2445 sec. 4.8.2.2 
     */
    QUrl dtend() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dtend", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dtend", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dtend. 
     * This property specifies the date and time that a calendar component 
     * ends. Inspired by RFC 2445 sec. 4.8.2.2 
     */
    void setDtend(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dtend", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dtend. 
     * This property specifies the date and time that a calendar component 
     * ends. Inspired by RFC 2445 sec. 4.8.2.2 
     */
    void addDtend(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#dtend", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfEventFreebusy", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
