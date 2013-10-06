/*
    Copyright (c) 2013 Mark Gaiser <markg85@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

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
