// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "singlefileresourceconfigquickbase.h"

class VCardConfigQuick : public SingleFileResourceConfigQuickBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the resource configuration plugin")
public:
    VCardConfigQuick(const KSharedConfigPtr &config, const QString &identifier, QObject *parent);
};
