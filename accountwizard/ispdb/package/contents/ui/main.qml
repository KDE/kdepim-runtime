// -*- coding: iso-8859-1 -*-
/*
 *   Author: Sebastian Kügler <sebas@kde.org>
 *   Date: Sun Nov 7 2010, 18:51:24
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import Qt 4.7
import org.kde.plasma.graphicswidgets 0.1 as PlasmaWidgets
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.graphicslayouts 4.7 as GraphicsLayouts

Item {
    width: 200
    height: 300

    PlasmaCore.DataSource {
          id: isdbSource
          engine: "org.kde.plasma.ispdb"
          interval: 0
          connectedSources: ["test@gmail.com"]
          onDataChanged: {
              console.log("data changed...");
          }
          onSourceAdded: {
             if (source != "networkStatus") {
                //console.log("QML addedd ......." + source)
                connectSource(source)
             }
          }
          onSourceRemoved: {
             if (source != "networkStatus") {
                //console.log("QML removed ......." + source)
                disconnectSource(source)
             }
          }
      }


    Row {
        id: searchRow
        width: parent.width
        PlasmaWidgets.IconWidget {
            id: icon
            onClicked: {
                timer.running = true
            }
        }
        PlasmaWidgets.LineEdit {
            id: searchBox
            text: "test@gmail.com"
            clickMessage: i18n("Type a word...");
            clearButtonShown: true
            width: parent.width - icon.width - parent.spacing
            onTextChanged: {
                timer.running = true
            }
        }
    }

    PlasmaWidgets.Label {
        id: mailAccount
        text: "<h3>" + isdbSource.data[searchBox.text]["longName"] + "</h3>"
        anchors { top: searchRow.bottom; left:parent.left; right: parent.right;}
    }

    ListView {
        anchors { top: mailAccount.bottom; left:parent.left; right: parent.right; bottom: parent.bottom }
        spacing: 8
        model: PlasmaCore.DataModel {
            dataSource: isdbSource
        }
        delegate: Text {
            id: citem
            //text: activatableType
          text: {
              print(searchBox.text + " == " + DataEngineSource);
              if (DataEngineSource != searchBox.text) {
                "<strong>" + protocol + " Server</strong>: " + username + "<br /> " + hostname + ":" + port + "<br />" + socketType+ " " + "/" + authType
              } else {
                longName
              }
          }
          //text: className
          Component.onCompleted: {
                print(" loaded item" + DataEngineSource);
          }
        }
    }
    Timer {
       id: timer
       running: false
       repeat: false
       interval: 3000
       onTriggered: {
            plasmoid.busy = true
            isdbSource.connectedSources = [searchBox.text]
       }
    }
}