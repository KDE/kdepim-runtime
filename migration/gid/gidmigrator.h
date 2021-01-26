/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GIDMIGRATOR_H
#define GIDMIGRATOR_H

#include <KJob>
#include <migratorbase.h>

class GidMigrator : public MigratorBase
{
    Q_OBJECT
public:
    GidMigrator(const QString &mimeType);
    ~GidMigrator() override;

    Q_REQUIRED_RESULT QString displayName() const override;
    Q_REQUIRED_RESULT QString description() const override;

    Q_REQUIRED_RESULT bool canStart() override;

    Q_REQUIRED_RESULT bool shouldAutostart() const override;

protected:
    void startWork() override;
private Q_SLOTS:
    void migrationFinished(KJob *);

private:
    const QString mMimeType;
};

#endif // GIDMIGRATOR_H
