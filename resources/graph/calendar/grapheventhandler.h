/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Maps between Graph `event` resources and KCalendarCore::Event. Read (toEvent) covers
    the common fields plus simple recurrences; write (toJson) mirrors them back.
    Times are requested from Graph in UTC (Prefer: outlook.timezone="UTC") so we never
    have to translate Windows time-zone names.
*/

#pragma once

#include <KCalendarCore/Event>
#include <QJsonObject>
#include <QString>

namespace GraphEventHandler
{
/// Akonadi item content mime type for calendar events.
[[nodiscard]] QString mimeType();

/// Graph event JSON -> KCalendarCore::Event (returns null on missing id).
[[nodiscard]] KCalendarCore::Event::Ptr toEvent(const QJsonObject &json);

/// KCalendarCore::Event -> Graph event JSON (for create/update).
[[nodiscard]] QJsonObject toJson(const KCalendarCore::Event::Ptr &event);

/// Apply a Graph patternedRecurrence to an incidence (shared with the todo handler).
void applyRecurrence(const QJsonObject &recurrence, const KCalendarCore::Incidence::Ptr &incidence);

/// Incidence recurrence -> Graph patternedRecurrence (empty if not recurring or the
/// rule has no Graph equivalent). Shared with the todo handler.
[[nodiscard]] QJsonObject recurrenceToJson(const KCalendarCore::Incidence::Ptr &incidence);
}
