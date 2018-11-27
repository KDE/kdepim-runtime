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

#include "formathelpers.h"
#include <klocalizedstring.h>
#include "kolabdefinitions.h"

namespace Kolab {
static const struct {
    const char *name;
    const char *label;
} folderTypeData[] = {
    { KOLAB_FOLDER_TYPE_MAIL, ""                      },
    { KOLAB_FOLDER_TYPE_CONTACT, I18N_NOOP("Contacts") },
    { KOLAB_FOLDER_TYPE_EVENT, I18N_NOOP("Calendar") },
    { KOLAB_FOLDER_TYPE_TASK, I18N_NOOP("Tasks")    },
    { KOLAB_FOLDER_TYPE_JOURNAL, I18N_NOOP("Journal")  },
    { KOLAB_FOLDER_TYPE_NOTE, I18N_NOOP("Notes")    },
    { KOLAB_FOLDER_TYPE_CONFIGURATION, I18N_NOOP("Configuration")    },
    { KOLAB_FOLDER_TYPE_FREEBUSY, I18N_NOOP("Freebusy") },
    { KOLAB_FOLDER_TYPE_FILE, I18N_NOOP("Files") }
};
static const int numFolderTypeData = sizeof folderTypeData / sizeof *folderTypeData;

std::string folderAnnotation(FolderType type, bool isDefault)
{
    Q_ASSERT(type >= 0 && type < LastType);
    std::string result = folderTypeData[ type ].name;
    if (isDefault) {
        result += KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX;
    }
    return result;
}

FolderType folderTypeFromString(const std::string &folderTypeName)
{
    if (folderTypeName == KOLAB_FOLDER_TYPE_CONTACT
        || folderTypeName == KOLAB_FOLDER_TYPE_CONTACT KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return ContactType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_EVENT
        || folderTypeName == KOLAB_FOLDER_TYPE_EVENT KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return EventType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_TASK
        || folderTypeName == KOLAB_FOLDER_TYPE_TASK KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return TaskType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_JOURNAL
        || folderTypeName == KOLAB_FOLDER_TYPE_JOURNAL KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return JournalType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_NOTE
        || folderTypeName == KOLAB_FOLDER_TYPE_NOTE KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return NoteType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_CONFIGURATION
        || folderTypeName == KOLAB_FOLDER_TYPE_CONFIGURATION KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return ConfigurationType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_FREEBUSY
        || folderTypeName == KOLAB_FOLDER_TYPE_FREEBUSY KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return FreebusyType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_FILE
        || folderTypeName == KOLAB_FOLDER_TYPE_FILE KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return FileType;
    }

    return MailType;
}

FolderType guessFolderTypeFromName(const std::string &name)
{
    for (int i = 0; i < numFolderTypeData; ++i) {
        if (name == i18n(folderTypeData[ i ].label).toStdString()
            || name == folderTypeData[ i ].label) {
            return static_cast<FolderType>(i);
        }
    }
    return MailType;
}

std::string nameForFolderType(FolderType type)
{
    Q_ASSERT(type >= 0 && type < LastType);
    return i18n(folderTypeData[ type ].label).toStdString();
}
}
