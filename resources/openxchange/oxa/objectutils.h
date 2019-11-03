/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef OXA_OBJECTUTILS_H
#define OXA_OBJECTUTILS_H

#include "folder.h"
#include "object.h"

class KJob;

class QDomDocument;
class QDomElement;

namespace OXA {
namespace ObjectUtils {
Object parseObject(const QDomElement &propElement, Folder::Module module);
void addObjectElements(QDomDocument &document, QDomElement &propElement, const Object &object, void *preloadedData = nullptr);

/**
 * Returns the dav path that is used for the given @p module.
 */
QString davPath(Folder::Module module);

/**
 * On some actions (e.g. creating or modifying items) we have to preload
 * data asynchronously. The following methods allow to do that in a generic way.
 */

/**
 * Checks whether the @p object needs preloading of data.
 */
bool needsPreloading(const Object &object);

/**
 * Creates a preloading job for the @p object.
 */
KJob *preloadJob(const Object &object);

/**
 * Converts the data loaded by the preloading @p job into pointer
 * that will be passed to addObjectElements later on.
 */
void *preloadData(const Object &object, KJob *job);
}
}

#endif
