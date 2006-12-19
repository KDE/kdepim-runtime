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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "core.h"

#include "pluginmanager.h"
#include "editor.h"
#include "plugin.h"

#include <ksettings/dialog.h>
#include <kplugininfo.h>
#include <kapplication.h>
#include <kconfig.h>
#include <ktrader.h>
#include <klibloader.h>
#include <kstandardaction.h>
#include <klistbox.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kshortcut.h>
#include <klocale.h>
#include <kstatusbar.h>
#include <kguiitem.h>
#include <kmenu.h>
#include <kshortcut.h>
#include <kcmultidialog.h>
#include <kaction.h>
#include <kstdaccel.h>
#include <kdebug.h>

#include <QWidget>

using namespace Komposer;

Core::Core( QWidget *parent, const char *name )
  : KomposerIface( "KomposerIface" ),
    KMainWindow( parent, name ), m_currentEditor( 0 )
{
  initWidgets();
  initCore();
  initConnections();
  setInstance( new KInstance( "komposer" ) );

  createActions();
  setXMLFile( "komposerui.rc" );

  createGUI( 0 );

  resize( 600, 400 ); // initial size
  setAutoSaveSettings();

  loadSettings();
}

Core::~Core()
{
  saveSettings();

  //Prefs::self()->writeConfig();
}

void
Core::addEditor( Komposer::Editor *editor )
{
  if ( editor->widget() ) {
    m_stack->addWidget( editor->widget() );
    m_stack->raiseWidget( editor->widget() );
    editor->widget()->show();
    m_currentEditor = editor;
  }

  // merge the editors GUI into the main window
  //insertChildClient( editor );
  guiFactory()->addClient( editor );
}

void
Core::addPlugin( Komposer::Plugin *plugin )
{
  //insertChildClient( plugin );
  guiFactory()->addClient( plugin );
}

void
Core::slotPluginLoaded( Plugin *plugin )
{
  kDebug() << "Plugin loaded "<<endl;

  Editor *editor = dynamic_cast<Editor*>( plugin );
  if ( editor ) {
    addEditor( editor );
  } else {
    addPlugin( plugin );
  }
}

void
Core::slotAllPluginsLoaded()
{
  QValueList<KPluginInfo*> plugins = m_pluginManager->availablePlugins();
  kDebug()<<"Number of available plugins is "<< plugins.count() <<endl;
  for ( QValueList<KPluginInfo*>::iterator it = plugins.begin(); it != plugins.end(); ++it ) {
    KPluginInfo *i = ( *it );
    kDebug()<<"\tAvailable plugin "<< i->pluginName()
             <<", comment = "<< i->comment() <<endl;
  }

  if ( !m_stack->visibleWidget() ) {
    m_pluginManager->loadPlugin( "komposer_defaulteditor", PluginManager::LoadAsync );
  }
}

#if 0
void
Core::slotActivePartChanged( KParts::Part *part )
{
  if ( !part ) {
    createGUI( 0 );
    return;
  }

  kDebug() << "Part activated: " << part << " with stack id. "
            << m_stack->id( part->widget() )<< endl;

  createGUI( part );
}

void
Core::selectEditor( Komposer::Editor *editor )
{
  if ( !editor )
    return;

  KParts::Part *part = editor->part();

  editor->select();

  QPtrList<KParts::Part> *partList = const_cast<QPtrList<KParts::Part>*>(
                                                   m_partManager->parts() );
  if ( partList->find( part ) == -1 )
    addPart( part );

  m_partManager->setActivePart( part );
  QWidget *view = part->widget();
  Q_ASSERT( view );

  kDebug()<<"Raising view "<<view<<endl;
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

}
#endif

void
Core::loadSettings()
{
  //kDebug()<<"Trying to select "<< Prefs::self()->m_activeEditor <<endl;
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
  kDebug()<<"exit"<<endl;
  m_pluginManager->shutdown();
}

void
Core::slotPreferences()
{
  if ( m_dlg == 0 )
    m_dlg = new KSettings::Dialog( this );
  m_dlg->show();
}

void
Core::initWidgets()
{
  statusBar()->show();
  QHBox *topWidget = new QHBox( this );
  setCentralWidget( topWidget );
  m_stack = new QWidgetStack( topWidget );
}

void
Core::initCore()
{
  m_pluginManager = new PluginManager( this );
  connect( m_pluginManager, SIGNAL(pluginLoaded(Plugin*)),
           SLOT(slotPluginLoaded(Plugin*)) );
  connect( m_pluginManager, SIGNAL(allPluginsLoaded()),
           SLOT(slotAllPluginsLoaded()) );


  m_pluginManager->loadAllPlugins();
  kDebug()<<"Loading"<<endl;
}

void
Core::initConnections()
{
  connect( kapp, SIGNAL(aboutToQuit()),
           SLOT(slotQuit()) );
}

void
Core::createActions()
{
  KStandardAction::close( this, SLOT(slotClose()), actionCollection() );

  (void) new KAction( i18n( "&Send" ), "mail_send", Qt::CTRL+Qt::Key_Return,
                      this, SLOT(slotSendNow()), actionCollection(),
                      "send_default" );

  (void) new KAction( i18n( "&Queue" ), "queue", 0,
                      this, SLOT(slotSendLater()),
                      actionCollection(), "send_alternative" );

  (void) new KAction( i18n( "Save in &Drafts Folder" ), "filesave", 0,
                      this, SLOT(slotSaveDraft()),
                      actionCollection(), "save_in_drafts" );
  (void) new KAction( i18n( "&Insert File..." ), "fileopen", 0,
                      this,  SLOT(slotInsertFile()),
                      actionCollection(), "insert_file" );
  (void) new KAction( i18n( "&Address Book" ), "contents",0,
                      this, SLOT(slotAddrBook()),
                      actionCollection(), "addressbook" );
  (void) new KAction( i18n( "&New Composer" ), "mail_new",
                      KStdAccel::shortcut( KStdAccel::New ),
                      this, SLOT(slotNewComposer()),
                      actionCollection(), "new_composer" );

  (void) new KAction( i18n( "&Attach File..." ), "attach",
                      0, this, SLOT(slotAttachFile()),
                      actionCollection(), "attach_file" );
}

void
Core::slotClose()
{
  close( false );
}

void
Core::slotSendNow()
{

}

void
Core::slotSendLater()
{

}

void
Core::slotSaveDraft()
{

}

void
Core::slotInsertFile()
{

}

void
Core::slotAddrBook()
{

}

void
Core::slotNewComposer()
{

}

void
Core::slotAttachFile()
{

}

void
Core::send( int how )
{

}

void
Core::addAttachment( const KUrl &url, const QString &comment )
{

}

void
Core::setBody( const QString &body )
{
  m_currentEditor->setText( body );
}

void
Core::addAttachment( const QString &name,
                     const QCString &cte,
                     const QByteArray &data,
                     const QCString &type,
                     const QCString &subType,
                     const QCString &paramAttr,
                     const QString &paramValue,
                     const QCString &contDisp )
{

}

#include "core.moc"
