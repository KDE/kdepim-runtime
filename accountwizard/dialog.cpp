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

#include <klocalizedstring.h>
#include <kross/core/action.h>
#include <kdebug.h>
#include "setuppage.h"

Dialog::Dialog(QWidget* parent) :
  KAssistantDialog( parent )
{
  SetupManager *setupManager = new SetupManager( this );
  ServerTest *serverTest = new ServerTest( this );
  Kross::Action* action = new Kross::Action( this, "AccountWizard" );
  action->addQObject( this, QLatin1String( "Dialog" ) );
  action->addQObject( setupManager, QLatin1String( "SetupManager" ) );
  action->addQObject( serverTest, QLatin1String( "ServerTest" ) );

  if ( Global::assistant().isEmpty() ) {
    PersonalDataPage *pdpage = new PersonalDataPage( this );
    addPage( pdpage, i18n( "Provide personal data" ) );
    connect( pdpage, SIGNAL( manualWanted( bool ) ), SLOT( slotManualConfigWanted( bool ) ) );

    TypePage* typePage = new TypePage( this );
    connect( typePage->treeview(), SIGNAL(doubleClicked(QModelIndex)), SLOT(slotNextPage()) );
    mTypePage = addPage( typePage, i18n( "Select Account Type" ) );
    setAppropriate( mTypePage, false );

#if KDE_IS_VERSION( 4, 4, 50 )
    ProviderPage *ppage = new ProviderPage( this );
    connect( ppage->treeview(), SIGNAL(doubleClicked(QModelIndex)), SLOT(slotNextPage()) );
    connect( ppage->advancedButton(), SIGNAL( clicked() ), SLOT( slotAdvancedWanted() ) );
    mProviderPage = addPage( ppage, i18n( "Select Provider" ) );
    setAppropriate( mProviderPage, false );
#endif
  }

  LoadPage *loadPage = new LoadPage( this );
  loadPage->setAction( action );
  mLoadPage = addPage( loadPage, i18n( "Loading Assistant" ) );
  setAppropriate( mLoadPage, false );

  SetupPage *setupPage = new SetupPage( this );
  mLastPage = addPage( setupPage, i18n( "Setting up Account" )  );
  setupManager->setSetupPage( setupPage );

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

void Dialog::slotAdvancedWanted() 
{
  Q_ASSERT( mTypePage );
  setAppropriate( mTypePage, true );
  //setCurrentPage( mTypePage ); // avoid the leavePage magic in the provider page
}

#include "dialog.moc"
