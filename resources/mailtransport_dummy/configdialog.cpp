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

#include <Collection>
#include <CollectionRequester>

#include <QDebug>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>

using namespace Akonadi;

ConfigDialog::ConfigDialog(QWidget * parent) :
    QDialog( parent )
{
  QWidget *mainWidget = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->addWidget(mainWidget);
  ui.setupUi(mainWidget);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  mOkButton = buttonBox->button(QDialogButtonBox::Ok);
  mOkButton->setDefault(true);
  mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);
  mainLayout->addWidget(buttonBox);


  ui.sink->setMimeTypeFilter( QStringList() << QLatin1String( "message/rfc822" ) );
  ui.sink->setAccessRightsFilter( Akonadi::Collection::CanCreateItem );
  // Don't bother fetching the collection. Will have an empty name :-/
  ui.sink->setCollection( Collection( Settings::self()->sink() ) );
  ui.sink->changeCollectionDialogOptions( Akonadi::CollectionDialog::AllowToCreateNewChildCollection );
  qDebug() << "Sink from settings" << Settings::self()->sink();

  connect(mOkButton, &QPushButton::clicked, this, &ConfigDialog::save);
  connect( ui.sink, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(slotCollectionChanged(Akonadi::Collection)) );
  mOkButton->setEnabled(false);
}

void ConfigDialog::slotCollectionChanged( const Akonadi::Collection& col )
{
  mOkButton->setEnabled(col.isValid());
}

void ConfigDialog::save()
{
  qDebug() << "Sink changed to" << ui.sink->collection().id();
  Settings::self()->setSink( ui.sink->collection().id() );
  Settings::self()->save();
}

