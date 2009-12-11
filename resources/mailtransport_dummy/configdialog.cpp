/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "configdialog.h"
#include "settings.h"

#include <Akonadi/Collection>
#include <Akonadi/CollectionRequester>

using namespace Akonadi;

ConfigDialog::ConfigDialog(QWidget * parent) :
    KDialog( parent )
{
  ui.setupUi( mainWidget() );

  ui.sink->setMimeTypeFilter( QStringList() << QLatin1String( "message/rfc822" ) );
  // Don't bother fetching the collection. Will have an empty name :-/
  ui.sink->setCollection( Collection( Settings::self()->sink() ) );
  kDebug() << "Sink from settings" << Settings::self()->sink();

  connect( this, SIGNAL(okClicked()), this, SLOT(save()) );
}

void ConfigDialog::save()
{
  kDebug() << "Sink changed to" << ui.sink->collection().id();
  Settings::self()->setSink( ui.sink->collection().id() );
  Settings::self()->writeConfig();
}

#include "configdialog.moc"
