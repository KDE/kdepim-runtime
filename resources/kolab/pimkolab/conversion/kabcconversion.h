/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef KOLABKABCCONVERSION_H
#define KOLABKABCCONVERSION_H

#include "kolab_export.h"

#include <kolabcontact.h>
#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>

namespace Kolab {
/**
 * Conversion of Kolab-Containers to/from KABC Containers.
 *
 */
namespace Conversion {
KOLAB_EXPORT KContacts::Addressee toKABC(const Kolab::Contact &);
KOLAB_EXPORT Kolab::Contact fromKABC(const KContacts::Addressee &);

KOLAB_EXPORT KContacts::ContactGroup toKABC(const Kolab::DistList &);
KOLAB_EXPORT Kolab::DistList fromKABC(const KContacts::ContactGroup &);
}
}

#endif
