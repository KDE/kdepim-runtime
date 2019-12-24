/*

    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ICALENDAR_H
#define ICALENDAR_H

#include "kolab_export.h"

#include <kolabevent.h>

namespace Kolab {
/**
 * Takes a list of events and writes them to an iCal object.
 *
 */
KOLAB_EXPORT std::string toICal(const std::vector<Kolab::Event> &);
/**
 * Takes an iCal object and returns the contained events.
 */
KOLAB_EXPORT std::vector<Kolab::Event> fromICalEvents(const std::string &);

class KOLAB_EXPORT ITipHandler
{
public:
    ITipHandler();
    enum ITipMethod {
        iTIPPublish,           /**< Event, to-do, journal or freebusy posting */
        iTIPRequest,           /**< Event, to-do or freebusy scheduling request */
        iTIPReply,             /**< Event, to-do or freebusy reply to request */
        iTIPAdd,               /**< Event, to-do or journal additional property request */
        iTIPCancel,            /**< Event, to-do or journal cancellation notice */
        iTIPRefresh,           /**< Event or to-do description update request */
        iTIPCounter,           /**< Event or to-do submit counter proposal */
        iTIPDeclineCounter,    /**< Event or to-do decline a counter proposal */
        iTIPNoMethod           /**< No method */
    };

    std::string toIMip(const Kolab::Event &, ITipMethod, const std::string &from, bool bbcMe = false) const;
    std::vector<Kolab::Event> fromIMip(const std::string &);

    /**
     * Create iTip message from single event
     */
    std::string toITip(const Kolab::Event &, ITipMethod) const;

    /**
     * Parse iTip message with a single event
     */
    std::vector<Kolab::Event> fromITip(const std::string &);
    ITipMethod method() const;
private:
    ITipMethod mMethod;
};
}

#endif // ICALENDAR_H
