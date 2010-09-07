/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Tom Albers <toma@kde.org>

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

#include "dialog.h"
#include "personaldatapage.h"
#include "providerpage.h"
#include "typepage.h"
#include "loadpage.h"
#include "global.h"
#include "dynamicpage.h"
#include "setupmanager.h"
#include "servertest.h"
#include "setuppage.h"

#include <KMime/Message>

#include <klocalizedstring.h>
#include <kross/core/action.h>
#include <kdebug.h>
#include <kmessagebox.h>

Dialog::Dialog(QWidget* parent) :
  KAssistantDialog( parent )
{
  mSetupManager = new SetupManager( this );
  ServerTest *serverTest = new ServerTest( this );
  Kross::Action* action = new Kross::Action( this, "AccountWizard" );
  action->addQObject( this, QLatin1String( "Dialog" ) );
  action->addQObject( mSetupManager, QLatin1String( "SetupManager" ) );
  action->addQObject( serverTest, QLatin1String( "ServerTest" ) );

  const bool showPersonalDataPage = Global::typeFilter().size() == 1 && Global::typeFilter().first() == KMime::Message::mimeType();

  if ( showPersonalDataPage ) {
    // todo: dont ask these details based on a setting of the desktop file.
    PersonalDataPage *pdpage = new PersonalDataPage( this );
    addPage( pdpage, i18n( "Provide personal data" ) );
    connect( pdpage, SIGNAL( manualWanted( bool ) ), SLOT( slotManualConfigWanted( bool ) ) );
    if ( !Global::assistant().isEmpty() ) {
      pdpage->setHideOptionInternetSearch( true );
    }
  }

  if ( Global::assistant().isEmpty() ) {
    TypePage* typePage = new TypePage( this );
    connect( typePage->treeview(), SIGNAL(doubleClicked(QModelIndex)), SLOT(slotNextPage()) );
    connect( typePage, SIGNAL( ghnsWanted() ), SLOT( slotGhnsWanted() ) );
    mTypePage = addPage( typePage, i18n( "Select Account Type" ) );
    setAppropriate( mTypePage, false );

    ProviderPage *ppage = new ProviderPage( this );
    connect( typePage, SIGNAL( ghnsWanted() ), ppage, SLOT(startFetchingData() ) );
    connect( ppage->treeview(), SIGNAL(doubleClicked(QModelIndex)), SLOT(slotNextPage()) );
    connect( ppage, SIGNAL( ghnsNotWanted() ), SLOT( slotGhnsNotWanted() ) );
    mProviderPage = addPage( ppage, i18n( "Select Provider" ) );
    setAppropriate( mProviderPage, false );
  } 

  LoadPage *loadPage = new LoadPage( this );
  loadPage->setAction( action );
  mLoadPage = addPage( loadPage, i18n( "Loading Assistant" ) );
  setAppropriate( mLoadPage, false );

  SetupPage *setupPage = new SetupPage( this );
  mLastPage = addPage( setupPage, i18n( "Setting up Account" )  );
  mSetupManager->setSetupPage( setupPage );

  slotManualConfigWanted( !showPersonalDataPage );

  Page *page = qobject_cast<Page*>( currentPage()->widget() );
  page->enterPageNext();
  emit page->pageEnteredNext();
  enableButton( KDialog::Help, false );

}

KPageWidgetItem* Dialog::addPage(Page* page, const QString &title)
{
  KPageWidgetItem *item = KAssistantDialog::addPage( page, title );
  connect( page, SIGNAL( leavePageNextOk() ), SLOT( slotNextOk() ) );
  connect( page, SIGNAL( leavePageBackOk() ), SLOT( slotBackOk() ) );
  page->setPageWidgetItem( item );
  return item;
}


void Dialog::slotNextPage()
{
  next();
}

void Dialog::next()
{
  Page *page = qobject_cast<Page*>( currentPage()->widget() );
  page->leavePageNext();
  page->leavePageNextRequested();
}

void Dialog::slotNextOk() 
{
  Page *page = qobject_cast<Page*>( currentPage()->widget() );
  emit page->pageLeftNext();
  KAssistantDialog::next();
  page = qobject_cast<Page*>( currentPage()->widget() );
  page->enterPageNext();
  emit page->pageEnteredNext();
}


void Dialog::back()
{
  Page *page = qobject_cast<Page*>( currentPage()->widget() );
  page->leavePageBack();
  page->leavePageBackRequested();
}

void Dialog::slotBackOk()
{
  Page *page = qobject_cast<Page*>( currentPage()->widget() );
  emit page->pageLeftBack();
  KAssistantDialog::back();
  page = qobject_cast<Page*>( currentPage()->widget() );
  page->enterPageBack();
  emit page->pageEnteredBack();
}

QObject* Dialog::addPage(const QString& uiFile, const QString &title )
{
  kDebug() << uiFile;
  DynamicPage *page = new DynamicPage( Global::assistantBasePath() + uiFile, this );
  connect( page, SIGNAL( leavePageNextOk() ), SLOT( slotNextOk() ) );
  connect( page, SIGNAL( leavePageBackOk() ), SLOT( slotBackOk() ) );
  KPageWidgetItem* item = insertPage( mLastPage, page, title );
  page->setPageWidgetItem( item );
  return page;
}

void Dialog::slotManualConfigWanted( bool show )
{
  Q_ASSERT( mTypePage );
  setAppropriate( mTypePage, show );
  setAppropriate( mLoadPage, show );
}

void Dialog::slotGhnsWanted() 
{
  Q_ASSERT( mProviderPage );
  setAppropriate( mProviderPage, true );
  setCurrentPage( mProviderPage );
}

void Dialog::slotGhnsNotWanted() 
{
  Q_ASSERT( mProviderPage );
  setAppropriate( mProviderPage, false );
}

SetupManager* Dialog::setupManager()
{
  return mSetupManager;
}

#include "dialog.moc"
