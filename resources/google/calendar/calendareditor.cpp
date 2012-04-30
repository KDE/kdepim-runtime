/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "calendareditor.h"
#include "ui_calendar_editor.h"

#include <libkgoogle/services/calendar.h>

#include <QtCore/QFile>
#include <KStandardDirs>
#include <KSystemTimeZone>

using namespace KGoogle::Objects;

CalendarEditor::CalendarEditor( Calendar *calendar ) :
  QDialog(),
  m_calendar( calendar )
{
  m_ui = new Ui::CalendarEditor();
  m_ui->setupUi( this );

  initTimezones();

  if ( calendar ) {
    initWidgets();
  } else {
    int i = m_ui->timezoneCombo->findText( KSystemTimeZones::local().name(), Qt::MatchContains );
    if ( i > -1 ) {
      m_ui->timezoneCombo->setCurrentIndex( i );
    }
  }

  connect( m_ui->buttons, SIGNAL( accepted() ),
           this, SLOT( accepted() ) );
}

CalendarEditor::~CalendarEditor()
{
  delete m_ui;
}

void CalendarEditor::accepted()
{
  if ( !m_calendar ) {
    m_calendar = new KGoogle::Objects::Calendar();
  }

  m_calendar->setTitle( m_ui->nameEdit->text() );
  m_calendar->setDetails( m_ui->descriptionEdit->toPlainText() );
  m_calendar->setLocation( m_ui->locationEdit->text() );
  m_calendar->setTimezone( m_ui->timezoneCombo->currentText() );

  Q_EMIT accepted( m_calendar );
}

void CalendarEditor::initTimezones()
{

  Q_FOREACH ( const KTimeZone &tz, KSystemTimeZones::zones() ) {
    QIcon icon;

    QString flag =
      KStandardDirs::locate( "locale",
                             QString( "l10n/%1/flag.png" ).arg( tz.countryCode().toLower() ) );

    if ( QFile::exists( flag ) ) {
      icon = QIcon( flag );
    }

    m_ui->timezoneCombo->addItem( icon, tz.name() );
  }
}

void CalendarEditor::initWidgets()
{

  m_ui->nameEdit->setText( m_calendar->title() );
  m_ui->descriptionEdit->setText( m_calendar->details() );
  m_ui->locationEdit->setText( m_calendar->location() );

  int tzIndex = m_ui->timezoneCombo->findText( m_calendar->timezone(), Qt::MatchContains );

  if ( tzIndex > -1 ) {
    m_ui->timezoneCombo->setCurrentIndex( tzIndex );
  }
}

