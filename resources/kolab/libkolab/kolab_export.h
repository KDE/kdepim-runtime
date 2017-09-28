/*
 *  This file is part of libkolab.
 *  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KOLAB_EXPORT_H
#define KOLAB_EXPORT_H

// #include <kdemacros.h>

#ifndef KOLAB_EXPORT
# if defined(KOLAB_STATIC_LIBS)
/* No export/import for static libraries */
#  define KOLAB_EXPORT
# elif defined(MAKE_KOLAB_LIB)
/* We are building this library */
#  define KOLAB_EXPORT __attribute__ ((visibility("default")))
# else
/* We are using this library */
#  define KOLAB_EXPORT __attribute__ ((visibility("default")))
# endif
#endif

# ifndef KOLAB_EXPORT_DEPRECATED
#  define KOLAB_EXPORT_DEPRECATED KDE_DEPRECATED __attribute__ ((visibility("default")))
# endif

/**
 *  @namespace Kolab
 * 
 *  @brief
 *  Contains all the KOLAB library global classes, objects, and functions.
 */

#endif
