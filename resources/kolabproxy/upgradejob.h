/*
 *  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 * 
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */


#ifndef UPGRADEJOB_H
#define UPGRADEJOB_H

#include <akonadi/job.h>
#include <akonadi/agentinstance.h>
#include <kolab/kolabobject.h>


/**
 * Fetch all items of an imap resource, read them, and write them out in the target version.
 */
class UpgradeJob : public Akonadi::Job
{
    Q_OBJECT
public:
    explicit UpgradeJob( Kolab::Version targetVersion, const Akonadi::AgentInstance &instance, QObject *parent = 0 );
    
protected:
    void doStart();
    
private slots:
    void collectionFetchResult( KJob *job );
    void itemFetchResult( KJob *job );
    void itemModifyResult( KJob *job );
    
private:
    Akonadi::AgentInstance m_agentInstance;
    Kolab::Version m_targetVersion;
};

#endif // UPGRADEJOB_H
