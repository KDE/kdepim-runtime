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
      var panel = children[i];
      panel.expanded.connect( this, collapseAll );
      panel.anchors.fill = _slideoutPanelContainer;
      panel.anchors.rightMargin = margin;
      panel.anchors.topMargin = margin;
      panel.anchors.bottomMargin = margin;
      if ( i >= 1 ) {
        var prevPanel = children[i - 1];
        panel.handlePosition = prevPanel.handlePosition + prevPanel.handleHeight
      }
    }
    // limit the height of the last panel to the available space
    if ( children.length > 0 ) {
      var lastPanel = children[ children.length - 1 ];
      if ( children.length > 1 ) {
        var prevPanel = children[ children.length - 2 ];
        lastPanel.handleHeight = Math.min( lastPanel.handleHeight, height - prevPanel.handlePosition - prevPanel.handleHeight - lastPanel.anchors.topMargin - lastPanel.anchors.bottomMargin );
      } else {
        lastPanel.handleHeight = Math.min( lastPanel.handleHeight, height - lastPanel.anchors.topMargin - lastPanel.anchors.bottomMargin );
      }
    }
  }
}
