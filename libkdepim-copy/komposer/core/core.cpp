// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
/**
 * core.cpp
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

#include "core.h"
#include "editor.h"

#include <ksettings/dialog.h>
#include <kparts/partmanager.h>
#include <kparts/part.h>
#include <kparts/componentfactory.h>
#include <kapplication.h>
#include <kconfig.h>
#include <ktrader.h>
#include <klibloader.h>
#include <kstdaction.h>
#include <klistbox.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kshortcut.h>
#include <kparts/componentfactory.h>
#include <klocale.h>
#include <kstatusbar.h>
#include <kguiitem.h>
#include <kpopupmenu.h>
#include <kshortcut.h>
#include <kcmultidialog.h>
#include <kdebug.h>

#include <qwidgetstack.h>
#include <qhbox.h>
#include <qwidget.h>

using namespace Komposer;

Core::Core( QWidget *parent, const char *name )
  : KParts::MainWindow( parent, name ), m_currentEditor( 0 )
{
  //m_editors.setAutoDelete( true );
  statusBar()->show();

  initWidgets();

  // prepare the part manager
  m_partManager = new KParts::PartManager( this );
  connect( m_partManager, SIGNAL(activePartChanged(KParts::Part*) ),
           this, SLOT(slotActivePartChanged(KParts::Part*)) );

  loadEditors();

  setXMLFile( "komposerui.rc" );

  createGUI( 0 );

  resize( 600, 400 ); // initial size
  setAutoSaveSettings();

  loadSettings();
}

Core::~Core()
{
  saveSettings();

  QPtrList<KParts::Part> parts = *m_partManager->parts();
  parts.setAutoDelete( true );
  parts.clear();

  //Prefs::self()->writeConfig();
}

void
Core::loadEditors()
{
  //m_pluginManager->loadEditors();
}

void
Core::unloadEditors()
{
}

void
Core::addEditor( Komposer::Editor *editor )
{
  kdDebug(5600) << "Added editor" << endl;

  // merge the editors GUI into the main window
  insertChildClient( editor );
}

void
Core::addPart( KParts::Part* part )
{
  kdDebug()<<"Part = "<< part << " widget = "<< part->widget() <<endl;
  if ( part->widget() )
    m_stack->addWidget( part->widget(), 0 );

  m_partManager->addPart( part, false );
}

void
Core::slotActivePartChanged( KParts::Part* part )
{
  if ( !part ) {
    createGUI( 0 );
    return;
  }

  kdDebug() << "Part activated: " << part << " with stack id. "
            << m_stack->id( part->widget() )<< endl;

  createGUI( part );
}
/*
void
Core::selectEditor( Komposer::Editor *editor )
{
  if ( !editor )
    return;

  KParts::Part *part = editor->part();

  editor->select();

  QPtrList<KParts::Part> *partList = const_cast<QPtrList<KParts::Part>*>( m_partManager->parts() );
  if ( partList->find( part ) == -1 )
    addPart( part );

  m_partManager->setActivePart( part );
  QWidget *view = part->widget();
  Q_ASSERT( view );

  kdDebug()<<"Raising view "<<view<<endl;
  if ( view )
  {
    m_stack->raiseWidget( view );
    view->show();
    view->setFocus();
    m_currentEditor = editor;
  }
}

void
Core::selectEditor( const QString &editorName )
{

}*/

void
Core::loadSettings()
{
  //kdDebug()<<"Trying to select "<< Prefs::self()->m_activeEditor <<endl;
  //selectEditor( Prefs::self()->m_activeEditor );

  //m_activeEditors = Prefs::self()->m_activeEditors;
}

void
Core::saveSettings()
{
  //if ( m_currentEditor )
    //Prefs::self()->m_activeEditor = m_currentEditor->identifier();
}

void
Core::slotQuit()
{
  close();
}

void
Core::slotPreferences()
{
  if ( m_dlg == 0 )
    m_dlg = new KSettings::Dialog( this );
  m_dlg->show();
}

KParts::ReadWritePart*
Core::createPart( const char *libname )
{
  kdDebug() << "Core:createPart(): " << libname << endl;

  QMap<QCString,KParts::ReadWritePart *>::ConstIterator it;

  it = m_parts.find( libname );

  if ( it != m_parts.end() )
    return it.data();

  kdDebug() << "Creating new KPart" << endl;

  KParts::ReadWritePart *part =
    KParts::ComponentFactory::
      createPartInstanceFromLibrary<KParts::ReadWritePart>
        ( libname, this, 0, this, "komposer" );

  if ( part )
    m_parts.insert( libname, part );

  return part;
}

void
Core::initWidgets()
{
  QHBox *topWidget = new QHBox( this );
  setCentralWidget( topWidget );
  m_stack = new QWidgetStack( topWidget );
}

#include "core.moc"
