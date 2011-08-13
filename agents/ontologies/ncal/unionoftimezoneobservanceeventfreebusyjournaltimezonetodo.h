#ifndef _NCAL_UNIONOFTIMEZONEOBSERVANCEEVENTFREEBUSYJOURNALTIMEZONETODO_H_
#define _NCAL_UNIONOFTIMEZONEOBSERVANCEEVENTFREEBUSYJOURNALTIMEZONETODO_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "ncal/unionparentclass.h"
namespace Nepomuk {
namespace NCAL {
/**
 * 
 */
class UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo : public NCAL::UnionParentClass
{
public:
    UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#comment. 
     * Non-processing information intended to provide a comment 
     * to the calendar user. Inspired by RFC 2445 sec. 4.8.1.4 with 
     * the following reservations: the LANGUAGE parameter has been 
     * discarded. Please use xml:lang literals to express language. 
     * For the ALTREP parameter use the commentAltRep property. 
     */
    QStringList comments() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#comment", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#comment. 
     * Non-processing information intended to provide a comment 
     * to the calendar user. Inspired by RFC 2445 sec. 4.8.1.4 with 
     * the following reservations: the LANGUAGE parameter has been 
     * discarded. Please use xml:lang literals to express language. 
     * For the ALTREP parameter use the commentAltRep property. 
     */
    void setComments(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#comment", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#comment. 
     * Non-processing information intended to provide a comment 
     * to the calendar user. Inspired by RFC 2445 sec. 4.8.1.4 with 
     * the following reservations: the LANGUAGE parameter has been 
     * discarded. Please use xml:lang literals to express language. 
     * For the ALTREP parameter use the commentAltRep property. 
     */
    void addComment(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#comment", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#commentAltRep. 
     * Alternate representation of the comment. Introduced to cover 
     * the ALTREP parameter of the COMMENT property. See documentation 
     * of ncal:comment for details. 
     */
    QList<QUrl> commentAltReps() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#commentAltRep", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#commentAltRep. 
     * Alternate representation of the comment. Introduced to cover 
     * the ALTREP parameter of the COMMENT property. See documentation 
     * of ncal:comment for details. 
     */
    void setCommentAltReps(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#commentAltRep", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#commentAltRep. 
     * Alternate representation of the comment. Introduced to cover 
     * the ALTREP parameter of the COMMENT property. See documentation 
     * of ncal:comment for details. 
     */
    void addCommentAltRep(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#commentAltRep", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfTimezoneObservanceEventFreebusyJournalTimezoneTodo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
