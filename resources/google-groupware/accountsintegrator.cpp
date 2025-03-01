/*
 *  SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "accountsintegrator.h"

AccountsIntegrator::AccountsIntegrator(GoogleResource *parent)
    : QObject(parent)
    , m_resource(parent)
{
}

void AccountsIntegrator::setAccount(const QDBusObjectPath &path)
{
    m_resource->setAccount(path);
}
