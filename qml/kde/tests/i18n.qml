import Qt 4.6
import org.kde 4.5

Rectangle {
     width: 100; height: 100; color: "blue"
     MouseArea {
         anchors.fill: parent
         acceptedButtons: Qt.LeftButton | Qt.RightButton
         onClicked: {
             console.log( "*** begin i18n test ***" );
             console.log( KDE.i18n( "single message" ) );
             console.log( KDE.i18na( "single message with arguments  %1 %2", ["bla", "foo"] ) );
             console.log( KDE.i18nc( "context", "message with context" ) );
             console.log( KDE.i18nca( "context", "message with context and arguments %1 %2 %3", [5, 3.1415, "string"] ) );
             console.log( KDE.i18np( "singular", "%1 plural", 1 ) );
             console.log( KDE.i18np( "singluar", "%1 plural", 2 ) );
             console.log( KDE.i18npa( "singluar with arg %2", "%1 plural with arg %2", 1, ["bla"] ) );
             console.log( KDE.i18npa( "singluar with arg %2", "%1 plural with arg %2", 2, ["bla"] ) );
             console.log( KDE.i18ncp( "context", "singular", "%1 plural", 1 ) );
             console.log( KDE.i18ncp( "context", "singluar", "%1 plural", 2 ) );
             console.log( KDE.i18ncpa( "context", "singluar with arg %2", "%1 plural with arg %2", 1, ["bla"] ) );
             console.log( KDE.i18ncpa( "context", "singluar with arg %2", "%1 plural with arg %2", 2, ["bla"] ) );
             console.log( "*** end i18n test ***" );
         }
     }
     Text {
        text: KDE.i18n( "Click to run test!" ); 
     }
 }
