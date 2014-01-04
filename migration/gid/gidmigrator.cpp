/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.


    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "gidmigrator.h"

#include <KLocalizedString>
#include "gidmigrationjob.h"

GidMigrator::GidMigrator(const QString &mimeType)
    :MigratorBase(QLatin1String("gidmigrator") + mimeType),
    mMimeType(mimeType)
{
}

GidMigrator::~GidMigrator()
{

}

QString GidMigrator::displayName() const
{
    return i18nc("Name of the GID Migrator (intended for advanced users).", "GID Migrator: %1", mMimeType);
}

QString GidMigrator::description() const
{
    return i18n("Ensures that all items with the mimetype %1 have a GID if a GID extractor is available.",mMimeType);
}

bool GidMigrator::canStart()
{
    return MigratorBase::canStart();
}

bool GidMigrator::shouldAutostart() const
{
    return true;
}

void GidMigrator::startWork()
{
    GidMigrationJob *job = new GidMigrationJob(QStringList() << mMimeType, this);
    connect(job, SIGNAL(result(KJob*)), this, SLOT(migrationFinished(KJob*)));
}

void GidMigrator::migrationFinished(KJob *job)
{
    if (job->error()) {
        emit message(Error, i18n("Migration failed: %1", job->errorString()));
        setMigrationState(Failed);
    } else {
        setMigrationState(Complete);
    }
}
