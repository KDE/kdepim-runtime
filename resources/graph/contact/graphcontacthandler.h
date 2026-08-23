/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Maps between Graph `contact` resources and KContacts::Addressee.
*/

#pragma once

#include <KContacts/Addressee>
#include <QJsonObject>
#include <QString>

namespace GraphContactHandler
{
/// Akonadi item content mime type for contacts.
[[nodiscard]] QString mimeType();

[[nodiscard]] KContacts::Addressee toAddressee(const QJsonObject &json);
[[nodiscard]] QJsonObject toJson(const KContacts::Addressee &addressee);
}
