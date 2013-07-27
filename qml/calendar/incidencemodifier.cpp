#include "incidencemodifier.h"

#include <QDebug>

#include <KCalCore/MemoryCalendar>
#include <KCalCore/Calendar>
#include <KCalCore/Event>
#include <Akonadi/Calendar/IncidenceChanger>
#include <Akonadi/Calendar/CalendarBase>
#include <Akonadi/Calendar/FetchJobCalendar>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>



IncidenceModifier::IncidenceModifier(QObject *parent)
    : QObject(parent)
    , m_id(-1)
    , m_item()
    , m_incidence()
{

}

void IncidenceModifier::setId(qint64 id)
{
    Akonadi::Item remoteItem = Akonadi::Item(id);

    Akonadi::ItemFetchJob* fetch = new Akonadi::ItemFetchJob(remoteItem, this);
    fetch->fetchScope().fetchFullPayload();

    QObject::connect(fetch, SIGNAL(itemsReceived(Akonadi::Item::List)), this, SLOT(slotItemsReceived(Akonadi::Item::List)));
}

void IncidenceModifier::save()
{
    if(m_id < 0) return;

    Akonadi::IncidenceChanger* changer = new Akonadi::IncidenceChanger(this);
    changer->modifyIncidence(m_item, m_incidence);

    qDebug() << "... IncidenceModifier::save";
}

QString IncidenceModifier::summary()
{
    return m_incidence->summary();
}

void IncidenceModifier::setSummary(const QString &summary)
{
    m_incidence->setSummary(summary);
    emit summaryChanged();
}

QString IncidenceModifier::description()
{
    return m_incidence->description();
}

void IncidenceModifier::setDescription(const QString &description)
{
    m_incidence->setDescription(description);
    emit descriptionChanged();
}

void IncidenceModifier::slotItemsReceived(const Akonadi::Item::List &items)
{
    qDebug() << "Items received.. " << items.count();
    if(items.count() == 1) {
        m_item = items.first();
        m_id = m_item.id();
        m_incidence = m_item.payload<KCalCore::Incidence::Ptr>();
        emit incidenceLoaded(m_id);

        /*
        KCalCore::Incidence::Ptr incidence = m_item.payload<KCalCore::Incidence::Ptr>();

        qDebug() << "Description: " << incidence->summary();
        qDebug() << "Readonly?: " << incidence->isReadOnly();


        incidence->setSummary("bla bla bla.. changed through C++ from QML!");


        Akonadi::IncidenceChanger* changer = new Akonadi::IncidenceChanger(this);
        changer->modifyIncidence(m_item, incidence);

        */
    }
}

void IncidenceModifier::slotAboutToStart()
{
    qDebug() << "IncidenceModifier::slotAboutToStart.. ";
}
