/**
 * prefsmodule.cpp
 *
 * Copyright (C)  2003-2004  Zack Rusin <zack@kde.org>
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

#include "prefsmodule.h"

#include <kaboutdata.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <klocale.h>
#include <ktrader.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qbuttongroup.h>

#include <kdepimmacros.h>

extern "C"
{
  KDE_EXPORT KCModule *create_komposerconfig( QWidget *parent, const char * ) {
    return new Komposer::PrefsModule( parent, "komposerprefs" );
  }
}
using namespace Komposer;

PrefsModule::PrefsModule( QWidget *parent, const char *name )
  : KPrefsModule( Komposer::Prefs::self(), parent, name )
{
  QVBoxLayout *topLayout = new QVBoxLayout( this );

  EditorSelection *editors = new EditorSelection( i18n( "Editors" ),
                                                  Komposer::Prefs::self()->m_activeEditor,
                                                  this );
  topLayout->addWidget( editors->groupBox() );

  addWid( editors );

  load();
}

const KAboutData*
PrefsModule::aboutData() const
{
  KAboutData *about = new KAboutData( I18N_NOOP( "komposerconfig" ),
                                      I18N_NOOP( "KDE Komposer" ),
                                      0, 0, KAboutData::License_LGPL,
                                      I18N_NOOP( "(c), 2003-2004 Zack Rusin" ) );

  about->addAuthor( "Zack Rusin", 0, "zack@kde.org" );;

  return about;
}


EditorSelection::EditorSelection( const QString &text, QString &reference,
                                  QWidget *parent )
  : m_reference( reference )
{
  m_box = new QGroupBox( 0, Qt::Vertical, text, parent );
  QVBoxLayout *boxLayout = new QVBoxLayout( m_box->layout() );
  boxLayout->setAlignment( Qt::AlignTop );

  m_editorsCombo = new KComboBox( m_box );
  boxLayout->addWidget( m_editorsCombo );

  connect( m_editorsCombo, SIGNAL(activated(const QString&)),
           SLOT(slotActivated(const QString&)) );
}

EditorSelection::~EditorSelection()
{
}

QGroupBox*
EditorSelection::groupBox()  const
{
  return m_box;
}

void
EditorSelection::readConfig()
{
  m_editorsCombo->clear();

  KTrader::OfferList editors = KTrader::self()->query(
    QString::fromLatin1( "Komposer/Editor" ) );
  KTrader::OfferList::ConstIterator it;
  int i = 0;
  for ( it = editors.begin(); it != editors.end(); ++it, ++i ) {
    if ( !(*it)->hasServiceType( QString::fromLatin1( "Komposer/Editor" ) ) )
      continue;

    QString name = (*it)->property( "X-KDE-KomposerIdentifier" ).toString();
    m_editorsCombo->insertItem( name );
    if ( m_reference.contains( name ) )
      m_editorsCombo->setCurrentItem( i );
  }
}

void EditorSelection::writeConfig()
{
  m_reference =  m_services[ m_editorsCombo->currentText()]->
                 property( "X-KDE-KomposerIdentifier" ).toString();
}

void
EditorSelection::slotActivated( const QString &editor )
{
  if ( !editor.isEmpty() )
    emit changed();
}

void
EditorSelection::setItem( const QString &str )
{
  for ( int i = 0; i < m_editorsCombo->count(); ++i ) {
    if ( m_editorsCombo->text( i ) == str ) {
      m_editorsCombo->setCurrentItem( i );
      break;
    }
  }
}

#include "prefsmodule.moc"
