/*
    This file is part of Akonadi Contact.

    Copyright (c) 2007-2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "contacteditordialog.h"

#include "collectioncombobox.h"
#include "collectionfiltermodel_p.h"
#include "contacteditor.h"
#include "editor/contacteditorwidget.h"

#include <akonadi/descendantsproxymodel.h>
#include <akonadi/item.h>

#include <kabc/addressee.h>

#include <klocale.h>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Akonadi;

class ContactEditorDialog::Private
{
  public:
    Private( ContactEditorDialog::Mode mode, QAbstractItemModel *collectionModel,
             AbstractContactEditorWidget *editorWidget, ContactEditorDialog *parent )
      : q( parent )
    {
      q->setCaption( mode == ContactEditorDialog::CreateMode ? i18n( "New Contact" ) : i18n( "Edit Contact" ) );
      q->setButtons( ContactEditorDialog::Ok | ContactEditorDialog::Cancel );

      QWidget *mainWidget = new QWidget( q );
      q->setMainWidget( mainWidget );

      QGridLayout *layout = new QGridLayout( mainWidget );

      mEditor = new ContactEditor( mode == ContactEditorDialog::CreateMode ? ContactEditor::CreateMode : ContactEditor::EditMode,
                                   editorWidget ? editorWidget : new ContactEditorWidget(), q );

      if ( mode == ContactEditorDialog::CreateMode ) {
        QLabel *label = new QLabel( i18n( "Add to:" ), mainWidget );

        // flatten the collection tree structure to a collection list
        Akonadi::DescendantsProxyModel *descendantModel = new Akonadi::DescendantsProxyModel( q );
        descendantModel->setSourceModel( collectionModel );

        // filter for collections that support contacts
        CollectionFilterModel *filterModel = new CollectionFilterModel( q );
        filterModel->addContentMimeTypeFilter( KABC::Addressee::mimeType() );
        filterModel->setRightsFilter( Akonadi::Collection::CanCreateItem );
        filterModel->setSourceModel( descendantModel );

        CollectionComboBox *box = new CollectionComboBox( mainWidget );
        box->setModel( filterModel );

        layout->addWidget( label, 0, 0 );
        layout->addWidget( box, 0, 1 );

        connect( box, SIGNAL( selectionChanged( const Akonadi::Collection& ) ),
                 mEditor, SLOT( setDefaultCollection( const Akonadi::Collection& ) ) );

        mEditor->setDefaultCollection( box->selectedCollection() );
      }

      layout->addWidget( mEditor, 1, 0, 1, 2 );
      layout->setColumnStretch( 1, 1 );

      connect( mEditor, SIGNAL( contactStored( const Akonadi::Item& ) ),
               q, SIGNAL( contactStored( const Akonadi::Item& ) ) );

      connect( q, SIGNAL( okClicked() ), q, SLOT( slotOkClicked() ) );
      connect( q, SIGNAL( cancelClicked() ), q, SLOT( slotCancelClicked() ) );

      q->setInitialSize( QSize( 550, 650 ) );
    }

    void slotOkClicked()
    {
      mEditor->saveContact();

      q->accept();
    }

    void slotCancelClicked()
    {
      q->reject();
    }

    ContactEditorDialog *q;
    ContactEditor *mEditor;
};

ContactEditorDialog::ContactEditorDialog( Mode mode, QAbstractItemModel *collectionModel, QWidget *parent )
  : KDialog( parent ), d( new Private( mode, collectionModel, 0, this ) )
{
}

ContactEditorDialog::ContactEditorDialog( Mode mode, QAbstractItemModel *collectionModel,
                                          AbstractContactEditorWidget *editorWidget, QWidget *parent )
  : KDialog( parent ), d( new Private( mode, collectionModel, editorWidget, this ) )
{
}

ContactEditorDialog::~ContactEditorDialog()
{
  delete d;
}

void ContactEditorDialog::setContact( const Akonadi::Item &contact )
{
  d->mEditor->loadContact( contact );
}

#include "contacteditordialog.moc"
