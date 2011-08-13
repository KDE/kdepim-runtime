#ifndef _NCAL_RECURRENCERULE_H_
#define _NCAL_RECURRENCERULE_H_

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
 * 
 */
class RecurrenceRule
{
public:
    RecurrenceRule(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~RecurrenceRule() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byday. 
     * Weekdays the recurrence should occur. Defined in RFC 2445 sec. 
     * 4.3.10 
     */
    QList<QUrl> bydays() const {
        QList<QUrl> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byday", QUrl::StrictMode)))
            value << v.value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byday. 
     * Weekdays the recurrence should occur. Defined in RFC 2445 sec. 
     * 4.3.10 
     */
    void setBydays(const QList<QUrl>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QUrl& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byday", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byday. 
     * Weekdays the recurrence should occur. Defined in RFC 2445 sec. 
     * 4.3.10 
     */
    void addByday(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byday", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byhour. 
     * Hour of recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    QList<qint64> byhours() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byhour", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byhour. 
     * Hour of recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    void setByhours(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byhour", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byhour. 
     * Hour of recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    void addByhour(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byhour", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byweekno. 
     * The number of the week an event should recur. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    QList<qint64> byweeknos() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byweekno", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byweekno. 
     * The number of the week an event should recur. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    void setByweeknos(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byweekno", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byweekno. 
     * The number of the week an event should recur. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    void addByweekno(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byweekno", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byyearday. 
     * Day of the year the event should occur. Defined in RFC 2445 sec. 
     * 4.3.10 
     */
    QList<qint64> byyeardays() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byyearday", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byyearday. 
     * Day of the year the event should occur. Defined in RFC 2445 sec. 
     * 4.3.10 
     */
    void setByyeardays(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byyearday", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byyearday. 
     * Day of the year the event should occur. Defined in RFC 2445 sec. 
     * 4.3.10 
     */
    void addByyearday(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byyearday", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysecond. 
     * Second of a recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    QList<qint64> byseconds() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysecond", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysecond. 
     * Second of a recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    void setByseconds(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysecond", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysecond. 
     * Second of a recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    void addBysecond(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysecond", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#until. 
     * The UNTIL rule part defines a date-time value which bounds the 
     * recurrence rule in an inclusive manner. If the value specified 
     * by UNTIL is synchronized with the specified recurrence, this 
     * date or date-time becomes the last instance of the recurrence. 
     * If specified as a date-time value, then it MUST be specified 
     * in an UTC time format. If not present, and the COUNT rule part 
     * is also not present, the RRULE is considered to repeat forever. 
     */
    QList<QDateTime> untils() const {
        QList<QDateTime> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#until", QUrl::StrictMode)))
            value << v.value<QDateTime>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#until. 
     * The UNTIL rule part defines a date-time value which bounds the 
     * recurrence rule in an inclusive manner. If the value specified 
     * by UNTIL is synchronized with the specified recurrence, this 
     * date or date-time becomes the last instance of the recurrence. 
     * If specified as a date-time value, then it MUST be specified 
     * in an UTC time format. If not present, and the COUNT rule part 
     * is also not present, the RRULE is considered to repeat forever. 
     */
    void setUntils(const QList<QDateTime>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QDateTime& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#until", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#until. 
     * The UNTIL rule part defines a date-time value which bounds the 
     * recurrence rule in an inclusive manner. If the value specified 
     * by UNTIL is synchronized with the specified recurrence, this 
     * date or date-time becomes the last instance of the recurrence. 
     * If specified as a date-time value, then it MUST be specified 
     * in an UTC time format. If not present, and the COUNT rule part 
     * is also not present, the RRULE is considered to repeat forever. 
     */
    void addUntil(const QDateTime& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#until", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysetpos. 
     * The BYSETPOS rule part specify values which correspond to the 
     * nth occurrence within the set of events specified by the rule. 
     * Valid values are 1 to 366 or -366 to -1. It MUST only be used in conjunction 
     * with another BYxxx rule part. For example "the last work day 
     * of the month" could be represented as: RRULE: FREQ=MONTHLY; 
     * BYDAY=MO, TU, WE, TH, FR; BYSETPOS=-1. Each BYSETPOS value 
     * can include a positive (+n) or negative (-n) integer. If present, 
     * this indicates the nth occurrence of the specific occurrence 
     * within the set of events specified by the rule. Defined in RFC 
     * 2445 sec. 4.3.10 
     */
    QList<qint64> bysetposes() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysetpos", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysetpos. 
     * The BYSETPOS rule part specify values which correspond to the 
     * nth occurrence within the set of events specified by the rule. 
     * Valid values are 1 to 366 or -366 to -1. It MUST only be used in conjunction 
     * with another BYxxx rule part. For example "the last work day 
     * of the month" could be represented as: RRULE: FREQ=MONTHLY; 
     * BYDAY=MO, TU, WE, TH, FR; BYSETPOS=-1. Each BYSETPOS value 
     * can include a positive (+n) or negative (-n) integer. If present, 
     * this indicates the nth occurrence of the specific occurrence 
     * within the set of events specified by the rule. Defined in RFC 
     * 2445 sec. 4.3.10 
     */
    void setBysetposes(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysetpos", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysetpos. 
     * The BYSETPOS rule part specify values which correspond to the 
     * nth occurrence within the set of events specified by the rule. 
     * Valid values are 1 to 366 or -366 to -1. It MUST only be used in conjunction 
     * with another BYxxx rule part. For example "the last work day 
     * of the month" could be represented as: RRULE: FREQ=MONTHLY; 
     * BYDAY=MO, TU, WE, TH, FR; BYSETPOS=-1. Each BYSETPOS value 
     * can include a positive (+n) or negative (-n) integer. If present, 
     * this indicates the nth occurrence of the specific occurrence 
     * within the set of events specified by the rule. Defined in RFC 
     * 2445 sec. 4.3.10 
     */
    void addBysetpos(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bysetpos", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#interval. 
     * The INTERVAL rule part contains a positive integer representing 
     * how often the recurrence rule repeats. The default value is 
     * "1", meaning every second for a SECONDLY rule, or every minute 
     * for a MINUTELY rule, every hour for an HOURLY rule, every day 
     * for a DAILY rule, every week for a WEEKLY rule, every month for 
     * a MONTHLY rule andevery year for a YEARLY rule. Defined in RFC 
     * 2445 sec. 4.3.10 
     */
    qint64 interval() const {
        qint64 value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#interval", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#interval", QUrl::StrictMode)).first().value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#interval. 
     * The INTERVAL rule part contains a positive integer representing 
     * how often the recurrence rule repeats. The default value is 
     * "1", meaning every second for a SECONDLY rule, or every minute 
     * for a MINUTELY rule, every hour for an HOURLY rule, every day 
     * for a DAILY rule, every week for a WEEKLY rule, every month for 
     * a MONTHLY rule andevery year for a YEARLY rule. Defined in RFC 
     * 2445 sec. 4.3.10 
     */
    void setInterval(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#interval", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#interval. 
     * The INTERVAL rule part contains a positive integer representing 
     * how often the recurrence rule repeats. The default value is 
     * "1", meaning every second for a SECONDLY rule, or every minute 
     * for a MINUTELY rule, every hour for an HOURLY rule, every day 
     * for a DAILY rule, every week for a WEEKLY rule, every month for 
     * a MONTHLY rule andevery year for a YEARLY rule. Defined in RFC 
     * 2445 sec. 4.3.10 
     */
    void addInterval(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#interval", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freq. 
     * Frequency of a recurrence rule. Defined in RFC 2445 sec. 4.3.10 
     */
    QUrl freq() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freq", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freq", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freq. 
     * Frequency of a recurrence rule. Defined in RFC 2445 sec. 4.3.10 
     */
    void setFreq(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freq", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freq. 
     * Frequency of a recurrence rule. Defined in RFC 2445 sec. 4.3.10 
     */
    void addFreq(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#freq", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#wkst. 
     * The day that's counted as the start of the week. It is used to disambiguate 
     * the byweekno rule. Defined in RFC 2445 sec. 4.3.10 
     */
    QUrl wkst() const {
        QUrl value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#wkst", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#wkst", QUrl::StrictMode)).first().value<QUrl>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#wkst. 
     * The day that's counted as the start of the week. It is used to disambiguate 
     * the byweekno rule. Defined in RFC 2445 sec. 4.3.10 
     */
    void setWkst(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#wkst", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#wkst. 
     * The day that's counted as the start of the week. It is used to disambiguate 
     * the byweekno rule. Defined in RFC 2445 sec. 4.3.10 
     */
    void addWkst(const QUrl& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#wkst", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#count. 
     * How many times should an event be repeated. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    qint64 count() const {
        qint64 value;
        if(m_res->contains(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#count", QUrl::StrictMode)))
            value = m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#count", QUrl::StrictMode)).first().value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#count. 
     * How many times should an event be repeated. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    void setCount(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        values << value;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#count", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#count. 
     * How many times should an event be repeated. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    void addCount(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#count", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonth. 
     * Number of the month of the recurrence. Valid values are integers 
     * from 1 (January) to 12 (December). Defined in RFC 2445 sec. 4.3.10 
     */
    QList<qint64> bymonths() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonth", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonth. 
     * Number of the month of the recurrence. Valid values are integers 
     * from 1 (January) to 12 (December). Defined in RFC 2445 sec. 4.3.10 
     */
    void setBymonths(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonth", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonth. 
     * Number of the month of the recurrence. Valid values are integers 
     * from 1 (January) to 12 (December). Defined in RFC 2445 sec. 4.3.10 
     */
    void addBymonth(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonth", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byminute. 
     * Minute of recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    QList<qint64> byminutes() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byminute", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byminute. 
     * Minute of recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    void setByminutes(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byminute", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byminute. 
     * Minute of recurrence. Defined in RFC 2445 sec. 4.3.10 
     */
    void addByminute(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#byminute", QUrl::StrictMode), value);
    }

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonthday. 
     * Day of the month when the event should recur. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    QList<qint64> bymonthdays() const {
        QList<qint64> value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonthday", QUrl::StrictMode)))
            value << v.value<qint64>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonthday. 
     * Day of the month when the event should recur. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    void setBymonthdays(const QList<qint64>& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const qint64& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonthday", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonthday. 
     * Day of the month when the event should recur. Defined in RFC 2445 
     * sec. 4.3.10 
     */
    void addBymonthday(const qint64& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#bymonthday", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#RecurrenceRule", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
