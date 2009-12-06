/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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
#include "typepage.h"
#include "loadpage.h"
#include "global.h"
#include "dynamicpage.h"
#include "setupmanager.h"

#include <klocalizedstring.h>
#include <kross/core/action.h>
#include <kdebug.h>
#include "setuppage.h"

Dialog::Dialog(QWidget* parent) :
  KAssistantDialog( parent )
{
  SetupManager *setupManager = new SetupManager( this );
  Kross::Action* action = new Kross::Action( this, "AccountWizard" );
  action->addQObject( this, QLatin1String( "Dialog" ) );
  action->addQObject( setupManager, QLatin1String( "SetupManager" ) );

  if ( Global::assistant().isEmpty() )
    addPage( new TypePage( this ), i18n( "Select Account Type" ) );

  LoadPage *loadPage = new LoadPage( this );
  loadPage->setAction( action );
  addPage( loadPage, i18n( "Loading Assistant" ) );

  SetupPage *setupPage = new SetupPage( this );
  mLastPage = addPage( setupPage, i18n( "Setting up Account" )  );
  setupManager->setSetupPage( setupPage );

  Page *page = qobject_cast<Page*>( currentPage()->widget() );
  page->enterPageNext();
  emit page->pageEnteredNext();
}


KPageWidgetItem* Dialog::addPage(Page* page, const QString &title)
{
  KPageWidgetItem *item = KAssistantDialog::addPage( page, title );
  page->setPageWidgetItem( item );
  return item;
}

void Dialog::next()
{
  Page *page = qobject_cast<Page*>( currentPage()->widget() );
  page->leavePageNext();
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
  KPageWidgetItem* item = insertPage( mLastPage, page, title );
  page->setPageWidgetItem( item );
  return page;
}

#include "dialog.moc"
