/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

class KJob;
#include <migratorbase.h>

class GidMigrator : public MigratorBase
{
    Q_OBJECT
public:
    explicit GidMigrator(const QString &mimeType);
    ~GidMigrator() override;

    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QString description() const override;

    [[nodiscard]] bool canStart() override;

    [[nodiscard]] bool shouldAutostart() const override;

protected:
    void startWork() override;
private Q_SLOTS:
    void migrationFinished(KJob *);

private:
    const QString mMimeType;
};
