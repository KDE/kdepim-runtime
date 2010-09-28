/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "kolabdefs.h"

#include <klocale.h>

#include <boost/static_assert.hpp>
#include <sys/stat.h>

using namespace Kolab;

static const struct {
  const char *name;
  const char *label;
} folderTypeData[] = {
  { KOLAB_FOLDER_TYPE_MAIL,    ""                      },
  { KOLAB_FOLDER_TYPE_CONTACT, I18N_NOOP( "Contacts" ) },
  { KOLAB_FOLDER_TYPE_EVENT,   I18N_NOOP( "Calendar" ) },
  { KOLAB_FOLDER_TYPE_TASK,    I18N_NOOP( "Tasks" )    },
  { KOLAB_FOLDER_TYPE_JOURNAL, I18N_NOOP( "Journal" )  },
  { KOLAB_FOLDER_TYPE_NOTE,    I18N_NOOP( "Notes" )    }
};
static const int numFolderTypeData = sizeof folderTypeData / sizeof *folderTypeData;

BOOST_STATIC_ASSERT( numFolderTypeData == Kolab::FolderTypeSize );

FolderType Kolab::folderTypeFromString(const QByteArray& folderTypeName)
{
  if ( folderTypeName == KOLAB_FOLDER_TYPE_CONTACT || folderTypeName == KOLAB_FOLDER_TYPE_CONTACT KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX )
    return Kolab::Contact;
  if ( folderTypeName == KOLAB_FOLDER_TYPE_EVENT   || folderTypeName == KOLAB_FOLDER_TYPE_EVENT   KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX )
    return Kolab::Event;
  if ( folderTypeName == KOLAB_FOLDER_TYPE_TASK    || folderTypeName == KOLAB_FOLDER_TYPE_TASK    KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX )
    return Kolab::Task;
  if ( folderTypeName == KOLAB_FOLDER_TYPE_JOURNAL || folderTypeName == KOLAB_FOLDER_TYPE_JOURNAL KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX )
    return Kolab::Journal;
  if ( folderTypeName == KOLAB_FOLDER_TYPE_NOTE    || folderTypeName == KOLAB_FOLDER_TYPE_NOTE    KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX )
    return Kolab::Note;
  return Kolab::Mail;
}

QByteArray Kolab::folderTypeToString(FolderType type, bool isDefault )
{
  Q_ASSERT( type >= 0 && type < FolderTypeSize );
  QByteArray result = folderTypeData[ type ].name;
  if ( isDefault )
    result += KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX;
  return result;
}

Kolab::FolderType Kolab::guessFolderTypeFromName(const QString& name)
{
  for ( int i = 0; i < numFolderTypeData; ++i ) {
    if ( name == i18n( folderTypeData[ i ].label ) || name == QString::fromLatin1( folderTypeData[ i ].label ) )
      return static_cast<FolderType>( i );
  }
  return Kolab::Mail;
}

QString Kolab::nameForFolderType(FolderType type)
{
  Q_ASSERT( type >= 0 && type < FolderTypeSize );
  return  i18n( folderTypeData[ type ].label );
}
