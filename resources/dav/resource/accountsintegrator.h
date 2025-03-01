/*
 *  SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDBusObjectPath>
#include <QObject>

#include "davgroupwareresource.h"

class AccountsIntegrator : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Accounts")

public:
    AccountsIntegrator(DavGroupwareResource *parent);

    Q_SCRIPTABLE void setAccount(const QDBusObjectPath &path);

private:
    DavGroupwareResource *m_resource;
};
