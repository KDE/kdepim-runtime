/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

import Qt 4.7
import org.kde 4.5
import org.kde.akonadi 4.5

BreadcrumbNavigationView {
  topDelegate : CollectionDelegate {
    indentation : 35
    fullClickArea : true
    topItem : true
    height : itemHeight
    onIndexSelected : {
      breadcrumbTopLevel._transitionSelect = -1;
      breadcrumbTopLevel.state = "before_select_home";
    }
  }

  breadcrumbDelegate : CollectionDelegate {
    indentation : 35
    fullClickArea : true
    height : itemHeight
    onIndexSelected : {
      breadcrumbTopLevel._transitionSelect = row;
      breadcrumbTopLevel.state = "before_select_breadcrumb";
    }
  }

  selectedItemDelegate : CollectionDelegate {
    indentation : 35
    height : itemHeight
    selectedDelegate : true
  }

  childItemsDelegate : CollectionDelegate {
    indentation : 35
    id : childDelegateWrapper
    height : itemHeight
    fullClickArea : true
    showChildIndicator : true
    onIndexSelected : {
      breadcrumbTopLevel._transitionSelect = row;
      breadcrumbTopLevel.state = "before_select_child";
    }
  }
}
