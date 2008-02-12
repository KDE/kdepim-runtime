/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <tomalbers@kde.nl>

    Based on KMail code by:
    Copyright (C) 2001-2003 Marc Mutz <mutz@kde.org>

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

#include "imaplibmanagementwidget.h"
#include "ui_imaplibmanagementwidget.h"

class ImaplibManagementWidget::Private
{
  public:
    Ui::ImaplibManagementWidget ui;
};

ImaplibManagementWidget::ImaplibManagementWidget(QWidget * parent) :
    QWidget( parent ),
    d( new Private )
{
  d->ui.setupUi( this );

  d->ui.transportList->setHeaderLabels(
                           QStringList() << i18n("Name") << i18n("Type") );
  connect( d->ui.transportList, SIGNAL(currentItemChanged(QTreeWidgetItem*,
           QTreeWidgetItem*)), SLOT(updateButtonState()) );
  connect( d->ui.transportList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
           SLOT(editClicked()) );
  connect( d->ui.addButton, SIGNAL(clicked()), SLOT(addClicked()) );
  connect( d->ui.editButton, SIGNAL(clicked()), SLOT(editClicked()) );
  connect( d->ui.removeButton, SIGNAL(clicked()), SLOT(removeClicked()) );
  connect( d->ui.defaultButton, SIGNAL(clicked()), SLOT(defaultClicked()) );

  /*fillImaplibList();
  connect( TransportManager::self(), SIGNAL(transportsChanged()),
           SLOT(fillTransportList()) ); */
}

ImaplibManagementWidget::~ImaplibManagementWidget()
{
  delete d;
}

void ImaplibManagementWidget::fillImaplibList()
{
}

void ImaplibManagementWidget::updateButtonState()
{
  if ( !d->ui.transportList->currentItem() ) {
    d->ui.editButton->setEnabled( false );
    d->ui.removeButton->setEnabled( false );
    d->ui.defaultButton->setEnabled( false );
  } else {
    d->ui.editButton->setEnabled( true );
    d->ui.removeButton->setEnabled( true );
//    if ( d->ui.transportList->currentItem()->data( 0, Qt::UserRole ) ==
//         TransportManager::self()->defaultTransportId() )
//      d->ui.defaultButton->setEnabled( false );
//    else
      d->ui.defaultButton->setEnabled( true );
  }
}

void ImaplibManagementWidget::addClicked()
{
}

void ImaplibManagementWidget::editClicked()
{
}

void ImaplibManagementWidget::removeClicked()
{
}

void ImaplibManagementWidget::defaultClicked()
{
}

#include "imaplibmanagementwidget.moc"
