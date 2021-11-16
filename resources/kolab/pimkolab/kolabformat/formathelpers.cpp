/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "formathelpers.h"
#include "ki18n_version.h"
#include "kolabdefinitions.h"
#include <KLocalizedString>
#if KI18N_VERSION >= QT_VERSION_CHECK(5, 89, 0)
#include <klazylocalizedstring.h>
#undef I18N_NOOP
#define I18N_NOOP kli18n
#endif
namespace Kolab
{
static const struct {
    const char *name;
#if KI18N_VERSION < QT_VERSION_CHECK(5, 89, 0)
    const char *label;
#else
    const KLazyLocalizedString label;
#endif
} folderTypeData[] = {
#if KI18N_VERSION < QT_VERSION_CHECK(5, 89, 0)
    {KOLAB_FOLDER_TYPE_MAIL, ""},
#else
    {KOLAB_FOLDER_TYPE_MAIL, KLazyLocalizedString()},
#endif
    {KOLAB_FOLDER_TYPE_CONTACT, I18N_NOOP("Contacts")},
    {KOLAB_FOLDER_TYPE_EVENT, I18N_NOOP("Calendar")},
    {KOLAB_FOLDER_TYPE_TASK, I18N_NOOP("Tasks")},
    {KOLAB_FOLDER_TYPE_JOURNAL, I18N_NOOP("Journal")},
    {KOLAB_FOLDER_TYPE_NOTE, I18N_NOOP("Notes")},
    {KOLAB_FOLDER_TYPE_CONFIGURATION, I18N_NOOP("Configuration")},
    {KOLAB_FOLDER_TYPE_FREEBUSY, I18N_NOOP("Freebusy")},
    {KOLAB_FOLDER_TYPE_FILE, I18N_NOOP("Files")}};
static const int numFolderTypeData = sizeof folderTypeData / sizeof *folderTypeData;

std::string folderAnnotation(FolderType type, bool isDefault)
{
    Q_ASSERT(type >= 0 && type < LastType);
    std::string result = folderTypeData[type].name;
    if (isDefault) {
        result += KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX;
    }
    return result;
}

FolderType folderTypeFromString(const std::string &folderTypeName)
{
    if (folderTypeName == KOLAB_FOLDER_TYPE_CONTACT || folderTypeName == KOLAB_FOLDER_TYPE_CONTACT KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return ContactType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_EVENT || folderTypeName == KOLAB_FOLDER_TYPE_EVENT KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return EventType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_TASK || folderTypeName == KOLAB_FOLDER_TYPE_TASK KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return TaskType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_JOURNAL || folderTypeName == KOLAB_FOLDER_TYPE_JOURNAL KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return JournalType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_NOTE || folderTypeName == KOLAB_FOLDER_TYPE_NOTE KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return NoteType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_CONFIGURATION || folderTypeName == KOLAB_FOLDER_TYPE_CONFIGURATION KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return ConfigurationType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_FREEBUSY || folderTypeName == KOLAB_FOLDER_TYPE_FREEBUSY KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return FreebusyType;
    }

    if (folderTypeName == KOLAB_FOLDER_TYPE_FILE || folderTypeName == KOLAB_FOLDER_TYPE_FILE KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX) {
        return FileType;
    }

    return MailType;
}

FolderType guessFolderTypeFromName(const std::string &name)
{
    for (int i = 0; i < numFolderTypeData; ++i) {
#if KI18N_VERSION < QT_VERSION_CHECK(5, 89, 0)
        if (name == i18n(folderTypeData[i].label).toStdString() || name == folderTypeData[i].label) {
            return static_cast<FolderType>(i);
        }
#else
        if (name == KLocalizedString(folderTypeData[i].label).toString().toStdString() || name == folderTypeData[i].label.untranslatedText()) {
            return static_cast<FolderType>(i);
        }
#endif
    }
    return MailType;
}

std::string nameForFolderType(FolderType type)
{
    Q_ASSERT(type >= 0 && type < LastType);
#if KI18N_VERSION < QT_VERSION_CHECK(5, 89, 0)
    return i18n(folderTypeData[type].label).toStdString();
#else
    return KLocalizedString(folderTypeData[type].label).toString().toStdString();
#endif
}
}
