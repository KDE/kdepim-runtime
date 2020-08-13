/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef KOLABCONVERSION_H
#define KOLABCONVERSION_H

#include "kolab_export.h"

#include <kolabnote.h>
#include <kmime/kmime_message.h>

namespace Kolab {
/**
 * Conversion of Kolab-Containers to/from KDE Containers.
 *
 */
namespace Conversion {
KOLAB_EXPORT KMime::Message::Ptr toNote(const Kolab::Note &);
KOLAB_EXPORT Kolab::Note fromNote(const KMime::Message::Ptr &);
}
}

#endif
