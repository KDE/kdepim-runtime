/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#ifndef KPIM_KCMDESIGNERFIELDS_H
#define KPIM_KCMDESIGNERFIELDS_H

#include <kcmodule.h>
#include <kdepimmacros.h>

class KListView;

class QLabel;
class QListViewItem;
class QPushButton;

namespace KPIM {

class KDE_EXPORT KCMDesignerFields : public KCModule
{
  Q_OBJECT

  public:
    KCMDesignerFields( QWidget *parent = 0, const char *name = 0 );

    virtual void load();
    virtual void save();
    virtual void defaults();

  protected:
    void        loadUiFiles();
    void        loadActivePages(const QStringList&);
    QStringList saveActivePages();

    virtual QString localUiDir() = 0;
    virtual QString uiPath() = 0;
    virtual void writeActivePages( const QStringList & ) = 0;
    virtual QStringList readActivePages() = 0;
    virtual QString applicationName() = 0;

  private slots:
    void updatePreview( QListViewItem* );
    void itemClicked( QListViewItem* );
    void startDesigner();
    void rebuildList();
    void deleteFile();
    void importFile();
    void delayedInit();

  private:
    void initGUI();

    KListView *mPageView;
    QLabel *mPagePreview;
    QLabel *mPageDetails;
    QPushButton *mDeleteButton;    
    QPushButton *mImportButton;
    QPushButton *mDesignerButton;
};

}

#endif
