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
 * A container for arbitrary content that can be expanded via dragging from a screen edge.
 * @param titleText Tab label in collapsed state
 * @param tilteIcon Tab icon path in collapsed state
 * @param handlePosition offset from the top for the tab handle in collapsed state
 * @param handleWidth Tab height in collapsed state
 * @param handleHeight Tab width in collapsed state
 * @param dragThreshold Drag distance needed before expanding
 * @param radius tab and panel corner radius
 * @param contentWidth width of the content area in expanded state
 * @signal expanded emitted when the panel is expanded
 * @slot collapse collapses the panel
 */
Item {

  id : _topLevel
  property alias titleText : collapseFlap.titleText
  property alias titleIcon : collapseFlap.titleIcon

  property alias collapsedContent : collapseFlap.contentArea
  property alias expandedContent : expandFlap.contentArea


  // Compat. Remove these.
  property alias content : expandFlap.contentArea
  property int handlePosition : 0
  property int handleHeight : 160
  property int contentWidth : expandFlap.width - 40 - collapsedWidth //: expandFlap.contentWidth

  property int collapsedPosition : handlePosition
  property int expandedPosition : 0

  property int collapsedWidth : 36

  property int collapsedHeight : handleHeight
  property int expandedHeight : height

  property real collapseThreshold : 1/4
  property real expandThreshold : 3/4

  property bool noCollapse : false

  onHandlePositionChanged : {
    parent.relayout();
  }
  onHandleHeightChanged : {
    parent.relayout();
  }
  onCollapsedHeightChanged : {
    parent.relayout();
  }
  onCollapsedPositionChanged : {
    parent.relayout();
  }

  signal extensionChanged(real extension)

  function changeExtension(extension)
  {

  }

  signal expanded(variant obj)
  signal collapsed(variant obj)

  z: 100
  x : 0
  y : 0

  function collapse() {
    collapseFlap.changeExtension(0.0)
  }

  function expand() {
    collapseFlap.changeExtension(1.0)
  }

  Flap {
    id : collapseFlap
    x : collapsedWidth - width
    y : collapsedPosition
    threshold : collapseThreshold

    leftBound : 0
    rightBound : width - collapsedWidth

    height : collapsedHeight
    topBackgroundImage : "flap-collapsed-top.png"
    midBackgroundImage : "flap-collapsed-mid.png"
    bottomBackgroundImage : "flap-collapsed-bottom.png"
    expander : true

    onExtensionChanged : {
      if (extension == 1.0)
      {
        _topLevel.noCollapse = true;
        _topLevel.z = 99;
        expanded(parent);
        _topLevel.noCollapse = false;
      }
      else if (extension == 0.0) {
        _topLevel.z = 100;
        collapsed(parent);
      }

      expandFlap.changeExtension(extension)
    }
  }

  Flap {
    id : expandFlap
    x : -width
    y : expandedPosition
    leftBound : 0
    rightBound : 40 + contentWidth + collapsedWidth
    height : expandedHeight
    threshold : expandThreshold

    contentWidth : _topLevel.contentWidth // width - 40 - collapsedWidth

    topBackgroundImage : "flap-expanded-top.png"
    midBackgroundImage : "flap-expanded-mid.png"
    bottomBackgroundImage : "flap-expanded-bottom.png"

    onExtensionChanged : {
      collapseFlap.changeExtension(extension)
    }
  }
}
