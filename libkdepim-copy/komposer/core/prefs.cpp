/**
 * prefs.cpp<2>
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 */

#include "prefs.h"

#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstaticdeleter.h>

using namespace Komposer;

Prefs *Prefs::s_instance = 0;

static KStaticDeleter<Prefs> insd;


Prefs::Prefs()
  : KPrefs( "komposerrc" )
{
  KPrefs::setCurrentGroup( "View" );

  addItemString( "ActiveEditor", m_activeEditor, "krichtext" );

  QStringList defaultEditors;
  defaultEditors << "krichtext";
  addItemStringList( "ActiveEditors", m_activeEditors, defaultEditors );
}

Prefs::~Prefs()
{
  if ( s_instance == this )
    s_instance = insd.setObject( 0 );
}

Prefs *Prefs::self()
{
  if ( !s_instance ) {
    insd.setObject( s_instance, new Prefs() );
    s_instance->readConfig();
  }

  return s_instance;
}
