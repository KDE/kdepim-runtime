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

#ifndef PROVIDERPAGE_H
#define PROVIDERPAGE_H

#include "page.h"
#include <QStandardItemModel>

#include "ui_providerpage.h"

namespace KNS3 {
  class Entry;
}

class ProviderPage : public Page
{
  Q_OBJECT
  public:
    explicit ProviderPage( KAssistantDialog* parent = 0 );

    virtual void leavePageNext();
    QTreeView *treeview() const;
    QPushButton *advancedButton() const;

  private slots:
    void selectionChanged();
    void ghnsClicked();

  private:
    Ui::ProviderPage ui;
    QStandardItemModel *m_model;
    QHash<QStandardItem*,KNS3::Entry> m_providerEntries;
};

#endif
