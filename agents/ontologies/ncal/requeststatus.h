#ifndef _NCAL_REQUESTSTATUS_H_
#define _NCAL_REQUESTSTATUS_H_

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
 * Request Status. A class that was introduced to provide a structure 
 * for the value of ncal:requestStatus property. See documentation 
 * for ncal:requestStatus for details. 
 */
class RequestStatus
{
public:
    RequestStatus(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~RequestStatus() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#requestStatusData. 
     * Additional data associated with a request status. Inspired 
     * by the third part of the structured value for the REQUEST-STATUS 
     * property defined in RFC 2445 sec. 4.8.8.2 ("Textual exception 
     * data. For example, the offending property name and value or 
     * complete property line") 
     */
    QStringList requestStatusDatas() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#requestStatusData", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#requestStatusData. 
     * Additional data associated with a request status. Inspired 
     * by the third part of the structured value for the REQUEST-STATUS 
     * property defined in RFC 2445 sec. 4.8.8.2 ("Textual exception 
     * data. For example, the offending property name and value or 
     * complete property line") 
     */
    void setRequestStatusDatas(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#requestStatusData", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#requestStatusData. 
     * Additional data associated with a request status. Inspired 
     * by the third part of the structured value for the REQUEST-STATUS 
     * property defined in RFC 2445 sec. 4.8.8.2 ("Textual exception 
     * data. For example, the offending property name and value or 
     * complete property line") 
     */
    void addRequestStatusData(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#requestStatusData", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#statusDescription. 
     * Longer return status description. Inspired by the second part 
     * of the structured value of the REQUEST-STATUS property defined 
     * in RFC 2445 sec. 4.8.8.2 
     */
    QString statusDescription() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#statusDescription", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#statusDescription", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#statusDescription. 
     * Longer return status description. Inspired by the second part 
     * of the structured value of the REQUEST-STATUS property defined 
     * in RFC 2445 sec. 4.8.8.2 
     */
    void setStatusDescription(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#statusDescription", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#statusDescription. 
     * Longer return status description. Inspired by the second part 
     * of the structured value of the REQUEST-STATUS property defined 
     * in RFC 2445 sec. 4.8.8.2 
     */
    void addStatusDescription(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#statusDescription", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#returnStatus. 
     * Short return status. Inspired by the first element of the structured 
     * value of the REQUEST-STATUS property described in RFC 2445 
     * sec. 4.8.8.2. The short return status is a PERIOD character 
     * (US-ASCII decimal 46) separated 3-tuple of integers. For example, 
     * "3.1.1". The successive levels of integers provide for a successive 
     * level of status code granularity. The following are initial 
     * classes for the return status code. Individual iCalendar object 
     * methods will define specific return status codes for these 
     * classes. In addition, other classes for the return status code 
     * may be defined using the registration process defined later 
     * in this memo. 1.xx - Preliminary success. This class of status 
     * of status code indicates that the request has request has been 
     * initially processed but that completion is pending. 2.xx -Successful. 
     * This class of status code indicates that the request was completed 
     * successfuly. However, the exact status code can indicate that 
     * a fallback has been taken. 3.xx - Client Error. This class of 
     * status code indicates that the request was not successful. 
     * The error is the result of either a syntax or a semantic error 
     * in the client formatted request. Request should not be retried 
     * until the condition in the request is corrected. 4.xx - Scheduling 
     * Error. This class of status code indicates that the request 
     * was not successful. Some sort of error occurred within the calendaring 
     * and scheduling service, not directly related to the request 
     * itself. 
     */
    QString returnStatus() const {
        QString value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#returnStatus", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#returnStatus", QUrl::StrictMode)).first().value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#returnStatus. 
     * Short return status. Inspired by the first element of the structured 
     * value of the REQUEST-STATUS property described in RFC 2445 
     * sec. 4.8.8.2. The short return status is a PERIOD character 
     * (US-ASCII decimal 46) separated 3-tuple of integers. For example, 
     * "3.1.1". The successive levels of integers provide for a successive 
     * level of status code granularity. The following are initial 
     * classes for the return status code. Individual iCalendar object 
     * methods will define specific return status codes for these 
     * classes. In addition, other classes for the return status code 
     * may be defined using the registration process defined later 
     * in this memo. 1.xx - Preliminary success. This class of status 
     * of status code indicates that the request has request has been 
     * initially processed but that completion is pending. 2.xx -Successful. 
     * This class of status code indicates that the request was completed 
     * successfuly. However, the exact status code can indicate that 
     * a fallback has been taken. 3.xx - Client Error. This class of 
     * status code indicates that the request was not successful. 
     * The error is the result of either a syntax or a semantic error 
     * in the client formatted request. Request should not be retried 
     * until the condition in the request is corrected. 4.xx - Scheduling 
     * Error. This class of status code indicates that the request 
     * was not successful. Some sort of error occurred within the calendaring 
     * and scheduling service, not directly related to the request 
     * itself. 
     */
    void setReturnStatus(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#returnStatus", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#returnStatus. 
     * Short return status. Inspired by the first element of the structured 
     * value of the REQUEST-STATUS property described in RFC 2445 
     * sec. 4.8.8.2. The short return status is a PERIOD character 
     * (US-ASCII decimal 46) separated 3-tuple of integers. For example, 
     * "3.1.1". The successive levels of integers provide for a successive 
     * level of status code granularity. The following are initial 
     * classes for the return status code. Individual iCalendar object 
     * methods will define specific return status codes for these 
     * classes. In addition, other classes for the return status code 
     * may be defined using the registration process defined later 
     * in this memo. 1.xx - Preliminary success. This class of status 
     * of status code indicates that the request has request has been 
     * initially processed but that completion is pending. 2.xx -Successful. 
     * This class of status code indicates that the request was completed 
     * successfuly. However, the exact status code can indicate that 
     * a fallback has been taken. 3.xx - Client Error. This class of 
     * status code indicates that the request was not successful. 
     * The error is the result of either a syntax or a semantic error 
     * in the client formatted request. Request should not be retried 
     * until the condition in the request is corrected. 4.xx - Scheduling 
     * Error. This class of status code indicates that the request 
     * was not successful. Some sort of error occurred within the calendaring 
     * and scheduling service, not directly related to the request 
     * itself. 
     */
    void addReturnStatus(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#returnStatus", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#RequestStatus", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
