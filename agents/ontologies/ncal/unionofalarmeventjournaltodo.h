#ifndef _NCAL_UNIONOFALARMEVENTJOURNALTODO_H_
#define _NCAL_UNIONOFALARMEVENTJOURNALTODO_H_

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
class UnionOfAlarmEventJournalTodo : public NCAL::UnionParentClass
{
public:
    UnionOfAlarmEventJournalTodo(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfAlarmEventJournalTodo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#description. 
     * A more complete description of the calendar component, than 
     * that provided by the ncal:summary property.Inspired by RFC 
     * 2445 sec. 4.8.1.5 with following reservations: the LANGUAGE 
     * parameter has been discarded. Please use xml:lang literals 
     * to express language. For the ALTREP parameter use the descriptionAltRep 
     * property. 
     */
    QStringList descriptions() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#description", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#description. 
     * A more complete description of the calendar component, than 
     * that provided by the ncal:summary property.Inspired by RFC 
     * 2445 sec. 4.8.1.5 with following reservations: the LANGUAGE 
     * parameter has been discarded. Please use xml:lang literals 
     * to express language. For the ALTREP parameter use the descriptionAltRep 
     * property. 
     */
    void setDescriptions(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#description", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#description. 
     * A more complete description of the calendar component, than 
     * that provided by the ncal:summary property.Inspired by RFC 
     * 2445 sec. 4.8.1.5 with following reservations: the LANGUAGE 
     * parameter has been discarded. Please use xml:lang literals 
     * to express language. For the ALTREP parameter use the descriptionAltRep 
     * property. 
     */
    void addDescription(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#description", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summaryAltRep. 
     * Alternate representation of the comment. Introduced to cover 
     * the ALTREP parameter of the SUMMARY property. See documentation 
     * of ncal:summary for details. 
     */
    QList<QUrl> summaryAltReps() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summaryAltRep", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summaryAltRep. 
     * Alternate representation of the comment. Introduced to cover 
     * the ALTREP parameter of the SUMMARY property. See documentation 
     * of ncal:summary for details. 
     */
    void setSummaryAltReps(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summaryAltRep", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summaryAltRep. 
     * Alternate representation of the comment. Introduced to cover 
     * the ALTREP parameter of the SUMMARY property. See documentation 
     * of ncal:summary for details. 
     */
    void addSummaryAltRep(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summaryAltRep", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attach. 
     * The property provides the capability to associate a document 
     * object with a calendar component. Defined in the RFC 2445 sec. 
     * 4.8.1.1 
     */
    QList<QUrl> attachs() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attach", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attach. 
     * The property provides the capability to associate a document 
     * object with a calendar component. Defined in the RFC 2445 sec. 
     * 4.8.1.1 
     */
    void setAttachs(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attach", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attach. 
     * The property provides the capability to associate a document 
     * object with a calendar component. Defined in the RFC 2445 sec. 
     * 4.8.1.1 
     */
    void addAttach(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#attach", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summary. 
     * Defines a short summary or subject for the calendar component. 
     * Inspired by RFC 2445 sec 4.8.1.12 with the following reservations: 
     * the LANGUAGE parameter has been discarded. Please use xml:lang 
     * literals to express language. For the ALTREP parameter use 
     * the summaryAltRep property. 
     */
    QStringList summarys() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summary", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summary. 
     * Defines a short summary or subject for the calendar component. 
     * Inspired by RFC 2445 sec 4.8.1.12 with the following reservations: 
     * the LANGUAGE parameter has been discarded. Please use xml:lang 
     * literals to express language. For the ALTREP parameter use 
     * the summaryAltRep property. 
     */
    void setSummarys(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summary", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summary. 
     * Defines a short summary or subject for the calendar component. 
     * Inspired by RFC 2445 sec 4.8.1.12 with the following reservations: 
     * the LANGUAGE parameter has been discarded. Please use xml:lang 
     * literals to express language. For the ALTREP parameter use 
     * the summaryAltRep property. 
     */
    void addSummary(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#summary", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#descriptionAltRep. 
     * Alternate representation of the calendar entity description. 
     * Introduced to cover the ALTREP parameter of the DESCRIPTION 
     * property. See documentation of ncal:description for details. 
     */
    QList<QUrl> descriptionAltReps() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#descriptionAltRep", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#descriptionAltRep. 
     * Alternate representation of the calendar entity description. 
     * Introduced to cover the ALTREP parameter of the DESCRIPTION 
     * property. See documentation of ncal:description for details. 
     */
    void setDescriptionAltReps(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#descriptionAltRep", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#descriptionAltRep. 
     * Alternate representation of the calendar entity description. 
     * Introduced to cover the ALTREP parameter of the DESCRIPTION 
     * property. See documentation of ncal:description for details. 
     */
    void addDescriptionAltRep(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#descriptionAltRep", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfAlarmEventJournalTodo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
