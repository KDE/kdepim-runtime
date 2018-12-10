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

#ifndef FORMATHELPERS_H
#define FORMATHELPERS_H

#include <kolab_export.h>
#include <string>

namespace Kolab {
enum FolderType {
    MailType = 0,
    ContactType,
    EventType,
    TaskType,
    JournalType,
    NoteType,
    ConfigurationType,
    FreebusyType,
    FileType,
    LastType
};

/**
 * Returns the FolderType from a KOLAB_FOLDER_TYPE_* folder type string
 */
KOLAB_EXPORT FolderType folderTypeFromString(const std::string &folderTypeName);
/**
 * Returns the annotation string for a folder
 */
KOLAB_EXPORT std::string folderAnnotation(FolderType type, bool isDefault = false);
/**
 * Guesses the folder type from a user visible string
 */
KOLAB_EXPORT FolderType guessFolderTypeFromName(const std::string &name);

/**
 * Returns a folder name for a type
 */
KOLAB_EXPORT std::string nameForFolderType(FolderType type);
}

#endif
