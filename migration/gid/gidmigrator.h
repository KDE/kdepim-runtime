/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef GIDMIGRATOR_H
#define GIDMIGRATOR_H

#include <migration/migratorbase.h>
#include <KJob>

class GidMigrator: public MigratorBase
{
    Q_OBJECT
public:
    GidMigrator(const QString &mimeType);
    virtual ~GidMigrator();

    virtual QString displayName() const;
    virtual QString description() const;

    virtual bool canStart();

    virtual bool shouldAutostart() const;

protected:
    virtual void startWork();
private slots:
    void migrationFinished(KJob *);
private:
    QString mMimeType;
};

#endif // GIDMIGRATOR_H
