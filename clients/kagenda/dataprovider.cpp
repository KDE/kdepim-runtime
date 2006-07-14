/*
    This file is part of Akonadi.

    Copyright (c) 2006 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "dataprovider.h"

#include "agendamodel.h"
#include "contactmodel.h"
#include "dummymodel.h"

#include <libakonadi/jobqueue.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/messagefetchjob.h>

#include <kcal/calendarlocal.h>
#include <kcal/icalformat.h>

#include <kmessagebox.h>
#include <klocale.h>

#include <QDebug>

DataProvider::DataProvider()
  : mAgendaModel( 0 ), mContactModel( 0 )
{
}

void DataProvider::setupEventData()
{
  mAgendaModel = new AgendaModel();
  mModel = mAgendaModel;

  addEvent( 2006, 1, 10, 12, 13, "Eins" );
  addEvent( 2006, 1, 10, 15, 20, "Zwei" );
  addEvent( 2006, 1, 11, 12, 13, "Drei" );
  addEvent( 2006, 1, 12, 12, 13, "Vier" );
  addEvent( 2006, 1, 11, 8, 13, "Fuenf" );
  addEvent( 2006, 1, 20, 11, 13, "Sechs" );
  addEvent( 2006, 1, 21, 11, 13, "Sieben" );
  addEvent( 2006, 1, 24, 5, 23, "Acht" );
  addEvent( 2006, 1, 26, 0, 23, "Neun" );

  for( int i = 0; i < 23; ++i ) {
    addEvent( 2006, 2, i, i, i+1, "Event " + QString( i ) );
  }
}

void DataProvider::addEvent( int year, int month, int day,
  int startHour, int endHour, const QString &title )
{
  if ( !mAgendaModel ) return;

  Event e;
  
  e.start = QDateTime( QDate( year, month, day ), QTime( startHour, 0 ) );
  e.end = QDateTime( QDate( year, month, day ), QTime( endHour, 0 ) );
  e.title = title;
  
  mAgendaModel->addEvent( e );
}

void DataProvider::setupContactData()
{
  mContactModel = new ContactModel();
  mModel = mContactModel;

  addContact( "Dummy User", "dummy@example.com" );
  addContact( "Doris Dauerwurst" );
  addContact( "Hans Wurst", "hw@abc.de", "+493012345" );
  addContact( "Herr A" );
  addContact( "Herr B" );
  addContact( "Herr C" );
  addContact( "Herr D" );
  addContact( "Herr E" );
  addContact( "Herr F" );
  addContact( "Herr G" );
  addContact( "Herr H" );
  addContact( "Klara Klossbruehe", "kk@abc.de" );
  addContact( "Klaas Klever" );
  addContact( "Kater Karlo" );
  addContact( "Petrosilius Zwackelmann" );
  addContact( "Rumplestilzchen" );
  addContact( "Sabine Sonnenschein", "sonnenschein@sabine.org" );
  addContact( "Sebastian Sauertop", "sauertopf@sebastian.org" );
  addContact( "Xerxes", "xerxes@example.com" );
  addContact( "Zacharias Zimplerlich" );
}

void DataProvider::setupDummyData()
{
  mModel = new DummyModel();
}

void DataProvider::addContact( const QString &name, const QString &email,
  const QString &phone )
{
  if ( !mContactModel ) return;

  Contact c;
  
  c.name = name;
  c.email = email;
  c.phone = phone;
  
  mContactModel->addContact( c );
}

StripeView::Model *DataProvider::model() const
{
  return mModel;
}

void DataProvider::loadFile()
{
  if ( mAgendaModel ) {
    mAgendaModel->clear();
    
    KCal::CalendarLocal cal( "UTC" );
    cal.load( "kagenda.ics" );
    
    KCal::Event::List events = cal.events();
    foreach( KCal::Event *event, events ) {
      Event e;
      e.title = event->summary();
      e.start = event->dtStart();
      e.end = event->dtEnd();
      
      mAgendaModel->addEvent( e );
    }
  }
}

void DataProvider::saveFile()
{
  if ( mAgendaModel ) {
    KCal::CalendarLocal cal( "UTC" );
    foreach( Event e, mAgendaModel->events() ) {
      KCal::Event *event = new KCal::Event();
      event->setSummary( e.title );
      event->setDtStart( e.start );
      event->setDtEnd( e.end );
      event->setFloats( false );
      cal.addEvent( event );
    }
    cal.save( "kagenda.ics" );
  }
  
  if ( mContactModel ) {
  }
}

void DataProvider::loadAkonadi()
{
  if ( mAgendaModel ) {
    PIM::MessageFetchJob job( "res2/foo2" );
    if ( !job.exec() ) {
      KMessageBox::error( 0, "Error fetching messages" );
    } else {
      
    }
  }
}

void DataProvider::saveAkonadi()
{
  if ( mAgendaModel ) {
//    PIM::JobQueue jobQueue( this );
    KCal::ICalFormat format;
    foreach( Event e, mAgendaModel->events() ) {
      KCal::Event *event = new KCal::Event();
      event->setSummary( e.title );
      event->setDtStart( e.start );
      event->setDtEnd( e.end );
      event->setFloats( false );
      QString ical = format.toICalString( event );
      PIM::ItemAppendJob *job = new PIM::ItemAppendJob( "res2/foo2", ical.toUtf8(),
        "text/calendar", this );
      if ( !job->exec() ) {
        KMessageBox::error( 0, i18n("Error") );
      }
//      jobQueue.addJob( job );
    }
#if 0
    if ( !jobQueue.exec() ) {
      KMessageBox::error( 0, i18n("Error saving to Akonadi") );
    }
#endif
  }
}

void DataProvider::load()
{
  loadFile();
}

void DataProvider::save()
{
  saveFile();
  qDebug() << "tick";
  saveAkonadi();
  qDebug() << "tack";
}

#include "dataprovider.moc"
