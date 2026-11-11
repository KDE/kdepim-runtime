/*
    SPDX-FileCopyrightText: 2026 The kdepim-runtime authors

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsoccurrenceexclusions.h"

#include <KCalendarCore/Calendar>
#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/Recurrence>

#include "ewsclient_debug.h"
#include "ewsitem.h"
#include "ewsoccurrence.h"

EwsCalendarIncidence ewsPickIncidence(const KCalendarCore::Calendar::Ptr &calendar)
{
    const auto events = calendar->events();
    if (events.isEmpty()) {
        return {};
    }

    for (const KCalendarCore::Event::Ptr &event : events) {
        if (!event->recurrenceId().isValid()) {
            return {event, true};
        }
    }

    return {events.first(), false};
}

void ewsPrepareForStorage(const EwsItem &item, EwsCalendarIncidence &picked)
{
    if (!picked.incidence) {
        return;
    }

    if (picked.isMaster) {
        ewsApplyExclusions(*picked.incidence, ewsExclusionDateTimes(item));
        return;
    }

    // The RECURRENCE-ID stays - it is what the viewer substitutes at the slot of the
    // master - while the rule would make the occurrence a series of its own.
    picked.incidence->clearRecurrence();
}

QList<QDateTime> ewsExclusionDateTimes(const EwsItem &item)
{
    QList<QDateTime> exclusions;

    // A cancelled occurrence is not delivered at all, so nothing else can fill its
    // slot and the series has to stop generating it. A moved one is delivered as an
    // item of its own and fills the slot itself - excluding it would remove the one
    // place it could still be drawn.
    const auto deletedOccurrences = item[EwsItemFieldDeletedOccurrences].value<EwsOccurrence::List>();
    for (const EwsOccurrence &occurrence : deletedOccurrences) {
        const QDateTime start = occurrence.start();
        if (!start.isValid()) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Skipping a cancelled occurrence without a valid start");
            continue;
        }
        if (!exclusions.contains(start)) {
            exclusions.append(start);
        }
    }

    return exclusions;
}

QList<EwsMovedOccurrence> ewsMovedOccurrences(const EwsItem &item)
{
    QList<EwsMovedOccurrence> movedOccurrences;

    const auto modifiedOccurrences = item[EwsItemFieldModifiedOccurrences].value<EwsOccurrence::List>();
    for (const EwsOccurrence &occurrence : modifiedOccurrences) {
        const EwsId occurrenceId = occurrence.itemId();
        if (occurrenceId.id().isEmpty()) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Skipping a moved occurrence without an item id");
            continue;
        }
        movedOccurrences.append(EwsMovedOccurrence{occurrenceId, occurrence.originalStart()});
    }

    return movedOccurrences;
}

void ewsApplyExclusions(KCalendarCore::Incidence &incidence, const QList<QDateTime> &exclusionDateTimes)
{
    if (exclusionDateTimes.isEmpty() || !incidence.recurs()) {
        return;
    }

    KCalendarCore::Recurrence *recurrence = incidence.recurrence();
    if (recurrence->recurReadOnly()) {
        return;
    }

    // Excluding the moment, not the date: an event may recur more than once a day.
    for (const QDateTime &exclusion : exclusionDateTimes) {
        recurrence->addExDateTime(exclusion);
    }
}
