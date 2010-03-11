import Qt 4.6
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
