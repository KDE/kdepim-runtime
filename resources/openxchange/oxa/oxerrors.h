/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2012 Marco Nelles <marco.nelles@credativ.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_OXERRORS_H
#define OXA_OXERRORS_H

#include <QString>

namespace OXA {
/**
 * Namespace that contains methods for handling OX errors.
 *
 * @author Marco Nelles <marco.nelles@credativ.com>
 */
namespace OXErrors {
enum EditErrorID {
    EditErrorUndefined = 0,
    ConcurrentModification,
    ObjectNotFound,
    NoPermissionForThisAction,
    ConflictsDetected,
    MissingMandatoryFields,
    AppointmentConflicts,
    InternalServerError
};

/**
 * Parse error id from edit error text string @p errorText
 */
EditErrorID getEditErrorID(const QString &errorText);
}
}

#endif
