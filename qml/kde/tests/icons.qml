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
import org.kde 4.5

Image {
     width: 256; height: 256;
     MouseArea {
         anchors.fill: parent
         acceptedButtons: Qt.LeftButton | Qt.RightButton
         onClicked: {
             console.log( "*** begin icon path test ***" );
             console.log( "icon 4: " + KDE.iconPath( "akonadi", 4 ) );
             console.log( "icon 15: " + KDE.iconPath( "akonadi", 15 ) );
             console.log( "icon 32: " + KDE.iconPath( "akonadi", 32 ) );
             console.log( "icon 65: " + KDE.iconPath( "akonadi", 65 ) );
             console.log( "icon 1000: " + KDE.iconPath( "akonadi", 1000 ) );
             console.log( "*** end icon path test ***" );
         }
     }

     source: KDE.iconPath( "akonadi", 256 );
 }
