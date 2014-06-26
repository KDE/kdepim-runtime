/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef GMAILRETRIEVECOLLECTIONSTASK_H
#define GMAILRETRIEVECOLLECTIONSTASK_H

#include <imap/retrievecollectionstask.h>

class GmailRetrieveCollectionsTask : public RetrieveCollectionsTask
{
    Q_OBJECT
public:
    explicit GmailRetrieveCollectionsTask(ResourceStateInterface::Ptr resource, QObject *parent = 0);

protected Q_SLOTS:
    void onMailBoxesReceived(const QList<KIMAP::MailBoxDescriptor> &descriptors, const QList<QList<QByteArray> > &flags);

};

#endif // GMAILRETRIEVECOLLECTIONSTASK_H
