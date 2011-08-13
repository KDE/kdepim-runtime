#ifndef _NCAL_UNIONOFEVENTJOURNALTODO_H_
#define _NCAL_UNIONOFEVENTJOURNALTODO_H_

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
class UnionOfEventJournalTodo : public NCAL::UnionParentClass
{
public:
    UnionOfEventJournalTodo(Nepomuk::SimpleResource* res)
      : NCAL::UnionParentClass(res), m_res(res)
    {}

    virtual ~UnionOfEventJournalTodo() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#class. 
     * Defines the access classification for a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.3 with the following reservations: 
     * this property has limited vocabulary. Possible values are: 
     * PUBLIC, PRIVATE and CONFIDENTIAL. The default is PUBLIC. Those 
     * values are expressed as instances of the AccessClassification 
     * class. The user may create his/her own if necessary. 
     */
    /*QUrl class() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#class", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#class", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }*/

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#class. 
     * Defines the access classification for a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.3 with the following reservations: 
     * this property has limited vocabulary. Possible values are: 
     * PUBLIC, PRIVATE and CONFIDENTIAL. The default is PUBLIC. Those 
     * values are expressed as instances of the AccessClassification 
     * class. The user may create his/her own if necessary. 
     */
    void setClass(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#class", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#class. 
     * Defines the access classification for a calendar component. 
     * Inspired by RFC 2445 sec. 4.8.1.3 with the following reservations: 
     * this property has limited vocabulary. Possible values are: 
     * PUBLIC, PRIVATE and CONFIDENTIAL. The default is PUBLIC. Those 
     * values are expressed as instances of the AccessClassification 
     * class. The user may create his/her own if necessary. 
     */
    void addClass(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#class", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exrule. 
     * This property defines a rule or repeating pattern for an exception 
     * to a recurrence set. Inspired by RFC 2445 sec. 4.8.5.2. 
     */
    QList<QUrl> exrules() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exrule", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exrule. 
     * This property defines a rule or repeating pattern for an exception 
     * to a recurrence set. Inspired by RFC 2445 sec. 4.8.5.2. 
     */
    void setExrules(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exrule", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exrule. 
     * This property defines a rule or repeating pattern for an exception 
     * to a recurrence set. Inspired by RFC 2445 sec. 4.8.5.2. 
     */
    void addExrule(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#exrule", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalRelation. 
     * A common superproperty for all types of ncal relations. It is 
     * not to be used directly. 
     */
    QStringList ncalRelations() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalRelation", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalRelation. 
     * A common superproperty for all types of ncal relations. It is 
     * not to be used directly. 
     */
    void setNcalRelations(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalRelation", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalRelation. 
     * A common superproperty for all types of ncal relations. It is 
     * not to be used directly. 
     */
    void addNcalRelation(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#ncalRelation", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#categories. 
     * Categories for a calendar component. Inspired by RFC 2445 sec 
     * 4.8.1.2 with the following reservations: The LANGUAGE parameter 
     * has been discarded. Please use xml:lang literals to express 
     * multiple languages. This property can specify multiple comma-separated 
     * categories. The order of categories doesn't matter. Please 
     * use a separate triple for each category. 
     */
    QStringList categorieses() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#categories", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#categories. 
     * Categories for a calendar component. Inspired by RFC 2445 sec 
     * 4.8.1.2 with the following reservations: The LANGUAGE parameter 
     * has been discarded. Please use xml:lang literals to express 
     * multiple languages. This property can specify multiple comma-separated 
     * categories. The order of categories doesn't matter. Please 
     * use a separate triple for each category. 
     */
    void setCategorieses(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#categories", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#categories. 
     * Categories for a calendar component. Inspired by RFC 2445 sec 
     * 4.8.1.2 with the following reservations: The LANGUAGE parameter 
     * has been discarded. Please use xml:lang literals to express 
     * multiple languages. This property can specify multiple comma-separated 
     * categories. The order of categories doesn't matter. Please 
     * use a separate triple for each category. 
     */
    void addCategories(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#categories", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#created. 
     * This property specifies the date and time that the calendar 
     * information was created by the calendar user agent in the calendar 
     * store. Note: This is analogous to the creation date and time 
     * for a file in the file system. Inspired by RFC 2445 sec. 4.8.7.1. 
     * Note that this property is a subproperty of nie:created. The 
     * domain of nie:created is nie:DataObject. It is not a superclass 
     * of UnionOf_Vevent_Vjournal_Vtodo, but since that union is 
     * conceived as an 'abstract' class, and in real-life all resources 
     * referenced by this property will also be DataObjects, than 
     * this shouldn't cause too much of a problem. Note that RFC allows 
     * ONLY UTC time values for this property. 
     */
    QDateTime created() const {
        QDateTime value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#created", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#created", QUrl::StrictMode)).first().value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#created. 
     * This property specifies the date and time that the calendar 
     * information was created by the calendar user agent in the calendar 
     * store. Note: This is analogous to the creation date and time 
     * for a file in the file system. Inspired by RFC 2445 sec. 4.8.7.1. 
     * Note that this property is a subproperty of nie:created. The 
     * domain of nie:created is nie:DataObject. It is not a superclass 
     * of UnionOf_Vevent_Vjournal_Vtodo, but since that union is 
     * conceived as an 'abstract' class, and in real-life all resources 
     * referenced by this property will also be DataObjects, than 
     * this shouldn't cause too much of a problem. Note that RFC allows 
     * ONLY UTC time values for this property. 
     */
    void setCreated(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#created", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#created. 
     * This property specifies the date and time that the calendar 
     * information was created by the calendar user agent in the calendar 
     * store. Note: This is analogous to the creation date and time 
     * for a file in the file system. Inspired by RFC 2445 sec. 4.8.7.1. 
     * Note that this property is a subproperty of nie:created. The 
     * domain of nie:created is nie:DataObject. It is not a superclass 
     * of UnionOf_Vevent_Vjournal_Vtodo, but since that union is 
     * conceived as an 'abstract' class, and in real-life all resources 
     * referenced by this property will also be DataObjects, than 
     * this shouldn't cause too much of a problem. Note that RFC allows 
     * ONLY UTC time values for this property. 
     */
    void addCreated(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#created", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sequence. 
     * This property defines the revision sequence number of the calendar 
     * component within a sequence of revisions. Inspired by RFC 2445 
     * sec. 4.8.7.4 
     */
    qint64 sequence() const {
        qint64 value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sequence", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sequence", QUrl::StrictMode)).first().value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sequence. 
     * This property defines the revision sequence number of the calendar 
     * component within a sequence of revisions. Inspired by RFC 2445 
     * sec. 4.8.7.4 
     */
    void setSequence(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sequence", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sequence. 
     * This property defines the revision sequence number of the calendar 
     * component within a sequence of revisions. Inspired by RFC 2445 
     * sec. 4.8.7.4 
     */
    void addSequence(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#sequence", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToChild. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided to introduce three different properties 
     * to express the values of that parameter. This property expresses 
     * the RELATED-TO property with RELTYPE=CHILD parameter. 
     */
    QStringList relatedToChilds() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToChild", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToChild. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided to introduce three different properties 
     * to express the values of that parameter. This property expresses 
     * the RELATED-TO property with RELTYPE=CHILD parameter. 
     */
    void setRelatedToChilds(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToChild", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToChild. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided to introduce three different properties 
     * to express the values of that parameter. This property expresses 
     * the RELATED-TO property with RELTYPE=CHILD parameter. 
     */
    void addRelatedToChild(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToChild", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToSibling. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided that it is more natural to introduce three 
     * different properties to express the values of that parameter. 
     * This property expresses the RELATED-TO property with RELTYPE=SIBLING 
     * parameter. 
     */
    QStringList relatedToSiblings() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToSibling", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToSibling. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided that it is more natural to introduce three 
     * different properties to express the values of that parameter. 
     * This property expresses the RELATED-TO property with RELTYPE=SIBLING 
     * parameter. 
     */
    void setRelatedToSiblings(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToSibling", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToSibling. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided that it is more natural to introduce three 
     * different properties to express the values of that parameter. 
     * This property expresses the RELATED-TO property with RELTYPE=SIBLING 
     * parameter. 
     */
    void addRelatedToSibling(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToSibling", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToParent. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided that it is more natural to introduce three 
     * different properties to express the values of that parameter. 
     * This property expresses the RELATED-TO property with no RELTYPE 
     * parameter (the default value is PARENT), or with explicit RELTYPE=PARENT 
     * parameter. 
     */
    QStringList relatedToParents() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToParent", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToParent. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided that it is more natural to introduce three 
     * different properties to express the values of that parameter. 
     * This property expresses the RELATED-TO property with no RELTYPE 
     * parameter (the default value is PARENT), or with explicit RELTYPE=PARENT 
     * parameter. 
     */
    void setRelatedToParents(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToParent", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToParent. 
     * The property is used to represent a relationship or reference 
     * between one calendar component and another. Inspired by RFC 
     * 2445 sec. 4.8.4.5. Originally this property had a RELTYPE parameter. 
     * It has been decided that it is more natural to introduce three 
     * different properties to express the values of that parameter. 
     * This property expresses the RELATED-TO property with no RELTYPE 
     * parameter (the default value is PARENT), or with explicit RELTYPE=PARENT 
     * parameter. 
     */
    void addRelatedToParent(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#relatedToParent", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#UnionOfEventJournalTodo", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
