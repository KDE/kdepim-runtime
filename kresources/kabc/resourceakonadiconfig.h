/*
    This file is part of libkabc.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef KABC_RESOURCEAKONADICONFIG_H
#define KABC_RESOURCEAKONADICONFIG_H

#include "kresources/configwidget.h"

namespace Akonadi {
  class CollectionView;
}

class KAction;
class QPushButton;

namespace KABC {

class ResourceAkonadiConfig : public KRES::ConfigWidget
{
  Q_OBJECT

  public:
    ResourceAkonadiConfig( QWidget *parent = 0 );

  public Q_SLOTS:
    void loadSettings( KRES::Resource *resource );
    void saveSettings( KRES::Resource *resource );

  private:
    Akonadi::CollectionView *mCollectionView;

    KAction *mCreateAction;
    KAction *mDeleteAction;
    KAction *mSyncAction;
    KAction *mSubscriptionAction;

    QPushButton *mCreateButton;
    QPushButton *mDeleteButton;
    QPushButton *mSyncButton;
    QPushButton *mSubscriptionButton;

  private Q_SLOTS:
    void updateCollectionButtonState();
};

}

#endif
