/*
    This file is part of libkresources.
    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>

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

#ifndef KRESOURCES_RESOURCESELECTDIALOG_H
#define KRESOURCES_RESOURCESELECTDIALOG_H

#include <qobject.h>
#include <qptrlist.h>
#include <qmap.h>

#include <kdialog.h>
class KListBox;

namespace KRES {

class Resource;

/**
 * Dialog for selecting a resource.
 * 
 * Example:
 *
 * <pre>
 * KABC::Resource *res = KABC::ResourceSelectDialog::getResource();
 * if ( !( res ) ) {
 *   // no resource selected
 * } else {
 *   // do something with resource
 * }
 * </pre>
 */
class ResourceSelectDialog : KDialog
{
  // Q_OBJECT

public:
  /**
   * Constructor.
   * @param ab     The address book you want to select the resource from
   * @param parent The parent widget
   * @param name   The name of the dialog
   */
  ResourceSelectDialog( QPtrList<Resource> list, QWidget *parent = 0,
      const char *name = 0);

//  ~ResourceSelectDialog();

  /**
   * Return selected resource.
   */
  Resource *resource();

  /**
   * Open a dialog showing the available resources and return the resource the
   * user has selected. Returns 0, if the dialog was canceled.
   */
  static Resource *getResource( QPtrList<Resource> list, QWidget *parent = 0 );

private:
  KListBox *mResourceId;

  QMap<int, Resource*> mResourceMap;
};

}
#endif
