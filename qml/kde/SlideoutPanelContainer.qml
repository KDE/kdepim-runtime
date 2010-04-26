/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

/**
 * Container for a set of SlideoutPanels.
 * It ensures that only one of them is expanded at any given time.
 */
Item {
  id: _slideoutPanelContainer

  /** Margin of the panels regarding the container on the three sides where it is not docked to the edge. */
  property int margin: 20

  /** Collapse all panels. */
  function collapseAll()
  {
    for ( var i = 0; i < children.length; ++i ) {
      children[i].collapse();
    }
  }

  Component.onCompleted:
  {
    for ( var i = 0; i < children.length; ++i ) {
      children[i].expanded.connect( this, collapseAll );
    }
  }

  onChildrenChanged:
  {
    var panel = children[children.length - 1];
    panel.anchors.fill = _slideoutPanelContainer;
    panel.anchors.rightMargin = margin;
    panel.anchors.topMargin = margin;
    panel.anchors.bottomMargin = margin;

    if ( children.length >= 2 ) {
      var prevPanel = children[children.length - 2];
      panel.handlePosition = prevPanel.handlePosition + prevPanel.handleHeight
    }
  }
}
