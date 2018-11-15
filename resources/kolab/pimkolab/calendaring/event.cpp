/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "event.h"
#include <icalendar/icalendar.h>
#include <kolabformat/kolabobject.h>
#include <conversion/kcalconversion.h>
#include <conversion/commonconversion.h>

#include <iostream>
#include <kolabformat.h>
#include <kolabevent_p.h>

namespace Kolab {
namespace Calendaring {
Event::Event()
    : Kolab::Event()
{
    setUid(Kolab::generateUID());
}

Event::Event(const Kolab::Event &e)
    : Kolab::Event(e)
{
}

Event::~Event()
{
}

bool Event::read(const std::string &string)
{
    const Kolab::Event &e = Kolab::readEvent(string, false);
    if (Kolab::error()) {
        return false;
    }
    Kolab::Event::operator=(e);
    return true;
}

std::string Event::write() const
{
    return Kolab::writeEvent(*this);
}

bool Event::fromMime(const std::string &input)
{
    KMime::Message::Ptr msg = KMime::Message::Ptr(new KMime::Message);
    msg->setContent(KMime::CRLFtoLF(Kolab::Conversion::fromStdString(input).toUtf8()));
    msg->parse();
    msg->content(KMime::ContentIndex());
    KolabObjectReader reader(msg);
    if (reader.getType() != EventObject) {
        std::cout << "not an event ";
        return false;
    }
    const Kolab::Event &e = Kolab::Conversion::fromKCalCore(*reader.getEvent());
    Kolab::Event::operator=(e);
    return true;
}

std::string Event::toMime() const
{
    return std::string(QString(KolabObjectWriter::writeEvent(Kolab::Conversion::toKCalCore(*this))->encodedContent()).toUtf8().constData());
}

bool Event::fromICal(const std::string &input)
{
    std::vector<Kolab::Event> list = fromICalEvents(input);
    if (list.size() != 1) {
        std::cout << "invalid number of events: " << list.size();
        return false;
    }
    Kolab::Event::operator=(list.at(0));
    return true;
}

std::string Event::toICal() const
{
    std::vector<Kolab::Event> list;
    list.push_back(*this);
    return Kolab::toICal(list);
}

bool Event::fromIMip(const std::string &input)
{
    const std::vector<Kolab::Event> list = mITipHandler.fromIMip(input);
    if (list.size() != 1) {
        std::cout << "invalid number of events: " << list.size();
        return false;
    }
    Kolab::Event::operator=(list.at(0));
    return true;
}

std::string Event::toIMip(ITipMethod method) const
{
    std::vector<Kolab::Event> list;
    list.push_back(*this);
    return mITipHandler.toIMip(*this, static_cast<ITipHandler::ITipMethod>(method), organizer().email());
}

Calendaring::Event::ITipMethod Event::getSchedulingMethod() const
{
    Q_ASSERT((int)iTIPPublish == (int)ITipHandler::iTIPPublish);
    Q_ASSERT((int)iTIPNoMethod == (int)ITipHandler::iTIPNoMethod);
    return static_cast<ITipMethod>(mITipHandler.method());
}

bool contains(const Kolab::ContactReference &delegatorRef, const std::vector <Kolab::ContactReference > &list)
{
    for (const Kolab::ContactReference &ref : list) {
        if (delegatorRef.uid() == ref.uid() || delegatorRef.email() == ref.email() || delegatorRef.name() == ref.name()) {
            return true;
        }
    }
    return false;
}

void Event::delegate(const std::vector< Attendee > &delegators, const std::vector< Attendee > &delegatees)
{
    //First build a list of attendee references, and insert any missing attendees
    std::vector<Kolab::Attendee *> delegateesRef;
    for (const Attendee &a : delegatees) {
        if (Attendee *attendee = getAttendee(a.contact())) {
            delegateesRef.push_back(attendee);
        } else {
            d->attendees.push_back(a);
            delegateesRef.push_back(&d->attendees.back());
        }
    }

    std::vector<Kolab::Attendee *> delegatorsRef;
    foreach (const Attendee &a, delegators) {
        if (Attendee *attendee = getAttendee(a.contact())) {
            delegatorsRef.push_back(attendee);
        } else {
            std::cout << "missing delegator";
        }
    }

    foreach (Attendee *delegatee, delegateesRef) {
        std::vector <Kolab::ContactReference > delegatedFrom = delegatee->delegatedFrom();
        foreach (Attendee *delegator, delegatorsRef) {
            //Set the delegator on each delegatee
            const ContactReference &delegatorRef = delegator->contact();
            if (!contains(delegatorRef, delegatedFrom)) {
                delegatedFrom.push_back(Kolab::ContactReference(Kolab::ContactReference::EmailReference, delegatorRef.email(), delegatorRef.name()));
            }

            //Set the delegatee on each delegator
            std::vector <Kolab::ContactReference > delegatedTo = delegator->delegatedTo();
            const ContactReference &delegaeeRef = delegatee->contact();
            if (!contains(delegaeeRef, delegatedTo)) {
                delegatedTo.push_back(Kolab::ContactReference(Kolab::ContactReference::EmailReference, delegaeeRef.email(), delegaeeRef.name()));
            }
            delegator->setDelegatedTo(delegatedTo);
        }
        delegatee->setDelegatedFrom(delegatedFrom);
    }
}

Attendee *Event::getAttendee(const ContactReference &ref)
{
    for (std::vector <Kolab::Attendee >::iterator it = d->attendees.begin(), end = d->attendees.end();
         it != end; ++it) {
        if (it->contact().uid() == ref.uid() || it->contact().email() == ref.email() || it->contact().name() == ref.name()) {
            return &*it;
        }
    }
    return nullptr;
}

Attendee Event::getAttendee(const std::string &s)
{
    foreach (const Attendee &a, attendees()) {
        if (a.contact().uid() == s || a.contact().email() == s || a.contact().name() == s) {
            return a;
        }
    }
    return Attendee();
}

cDateTime Calendaring::Event::getNextOccurence(const cDateTime &date)
{
    KCalCore::Event::Ptr event = Kolab::Conversion::toKCalCore(*this);
    if (!event->recurs()) {
        return cDateTime();
    }
    const QDateTime nextDate = event->recurrence()->getNextDateTime(Kolab::Conversion::toDate(date));
    return Kolab::Conversion::fromDate(nextDate, date.isDateOnly());
}

cDateTime Calendaring::Event::getOccurenceEndDate(const cDateTime &startDate)
{
    KCalCore::Event::Ptr event = Kolab::Conversion::toKCalCore(*this);
    const QDateTime start = Kolab::Conversion::toDate(startDate);
    return Kolab::Conversion::fromDate(event->endDateForStart(start), event->allDay());
}

cDateTime Calendaring::Event::getLastOccurrence() const
{
    KCalCore::Event::Ptr event = Kolab::Conversion::toKCalCore(*this);
    if (!event->recurs()) {
        return cDateTime();
    }
    const QDateTime endDate = event->recurrence()->endDateTime();
    return Kolab::Conversion::fromDate(endDate, event->allDay());
}
}
}
