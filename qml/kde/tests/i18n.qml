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

Rectangle {
     width: 100; height: 100; color: "blue"
     MouseArea {
         anchors.fill: parent
         acceptedButtons: Qt.LeftButton | Qt.RightButton
         onClicked: {
             console.log( "*** begin i18n test ***" );
             console.log( KDE.i18n( "single message" ) );
             console.log( KDE.i18n( "single message with arguments  %1 %2", "bar", "foo" ) );
             console.log( KDE.i18n( "single message with arguments multiline %1 %2",
                                    "bar",
                                    "foo" ) );
             console.log( KDE.i18nc( "context", "message with context and \n to break our script" ) );
             console.log( KDE.i18nc( "context", "message with context and arguments %1 %2 %3", 5, 3.1415, "string" ) );
             console.log( KDE.i18np( "singular", "%1 plural", 1 ) );
             console.log( KDE.i18np( "singular", "%1 plural", 2 ) );
             console.log( KDE.i18np( "singular with arg %2", "%1 plural with arg %2", 1, "bla" ) );
             console.log( KDE.i18np( "singular with arg %2", "%1 plural with arg %2", 2, "bla" ) );
             console.log( KDE.i18ncp( "context", "singular", "%1 plural", 1 ) );
             console.log( KDE.i18ncp( "context", "singular", "%1 plural", 2 ) );
             console.log( KDE.i18ncp( "context", "singular with arg %2", "%1 plural with arg %2", 1, "bla" ) );
             console.log( KDE.i18ncp( "context", "singular with arg %2", "%1 plural with arg %2", 2, "bla" ) );
             console.log( "*** end i18n test ***" );
         }
     }
     Text {
        text: KDE.i18n( "Click to run test!" ); 
     }
 }
