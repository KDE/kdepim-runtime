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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
        
    };
};

#endif
