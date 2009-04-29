/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KRES_AKONADI_RESOURCECONFIGBASE_H
#define KRES_AKONADI_RESOURCECONFIGBASE_H

#include <kresources/configwidget.h>

#include <akonadi/collection.h>

namespace Akonadi {
  class CollectionView;
}

class KAction;
class KDialog;
class QLabel;
class QModelIndex;
class QPushButton;

class ResourceConfigBase : public KRES::ConfigWidget
{
  Q_OBJECT

  public:
    ResourceConfigBase( const QStringList &mimeList, QWidget *parent = 0 );

    ~ResourceConfigBase();

  public Q_SLOTS:
    void loadSettings( KRES::Resource *resource );

    void saveSettings( KRES::Resource *resource );

  protected:
    const QStringList mMimeList;

    Akonadi::Collection mCollection;

    Akonadi::CollectionView *mCollectionView;

    KAction *mCreateAction;
    KAction *mDeleteAction;
    KAction *mSyncAction;
    KAction *mSubscriptionAction;

    QPushButton *mCreateButton;
    QPushButton *mDeleteButton;
    QPushButton *mSyncButton;
    QPushButton *mSubscriptionButton;

    QLabel *mInfoTextLabel;

    KDialog     *mSourcesDialog;
    QPushButton *mSourcesButton;

  private Q_SLOTS:
    void updateCollectionButtonState();

    void collectionChanged( const Akonadi::Collection &collection );

    void collectionsInserted( const QModelIndex &parent, int start, int end );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
