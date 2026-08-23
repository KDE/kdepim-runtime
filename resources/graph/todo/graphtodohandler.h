/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Maps between Graph `todoTask` resources (Microsoft To Do) and KCalendarCore::Todo.
    Task times come as a naive local timestamp plus a separate timeZone field (the
    To Do API ignores the Prefer: outlook.timezone header), so parsing resolves the
    zone explicitly.
*/

#pragma once

#include <KCalendarCore/Todo>
#include <QJsonObject>
#include <QString>

namespace GraphTodoHandler
{
/// Akonadi item content mime type for todos.
[[nodiscard]] QString mimeType();

/// Graph todoTask JSON -> KCalendarCore::Todo (returns null on missing id).
[[nodiscard]] KCalendarCore::Todo::Ptr toTodo(const QJsonObject &json);

/// KCalendarCore::Todo -> Graph todoTask JSON (for create/update).
[[nodiscard]] QJsonObject toJson(const KCalendarCore::Todo::Ptr &todo);
}
