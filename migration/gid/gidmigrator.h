/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GIDMIGRATOR_H
#define GIDMIGRATOR_H

#include <migratorbase.h>
#include <KJob>

class GidMigrator : public MigratorBase
{
    Q_OBJECT
public:
    GidMigrator(const QString &mimeType);
    ~GidMigrator() override;

    QString displayName() const override;
    QString description() const override;

    bool canStart() override;

    bool shouldAutostart() const override;

protected:
    void startWork() override;
private Q_SLOTS:
    void migrationFinished(KJob *);
private:
    QString mMimeType;
};

#endif // GIDMIGRATOR_H
