/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef MESSAGEHELPER_H
#define MESSAGEHELPER_H

#include <Akonadi/Item>
#include <KMime/Message>
#include <KIMAP/FetchJob>
#include <boost/shared_ptr.hpp>

class MessageHelper {
public:
    typedef boost::shared_ptr<MessageHelper> Ptr;

    virtual ~MessageHelper();
    virtual Akonadi::Item createItemFromMessage(KMime::Message::Ptr message,
                                                const qint64 uid,
                                                const qint64 size,
                                                const QList<KIMAP::MessageAttribute> &attrs,
                                                const QList<QByteArray> &flags,
                                                const KIMAP::FetchJob::FetchScope &scope,
                                                bool &ok) const;
};

#endif
