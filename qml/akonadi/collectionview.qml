import Qt 4.6
import org.kde 4.5

/** Akonadi Collection List View
    @param model: The collection model to display, ETM-based, filtered to only contain a flat collection list.
    @param currentIndex: Index of the currently selected row.
 */
Rectangle {
  property alias model: collectionListView.model
  property alias currentIndex: collectionListView.currentIndex

  SystemPalette { id: palette; colorGroup: Qt.Active }
  Component {
    id: collectionViewDelegate

    Item {
      id: wrapper
      width: collectionListView.width
      height : 68

      Rectangle {
        id: background
        opacity: 0.25
        x: 1; y: 2; width: parent.width - 2; height: parent.height - 4
        border.color: palette.mid
        radius: 5
      }
      MouseArea {
          id: pageMouse
          anchors.fill: parent
          onClicked: {
            wrapper.ListView.view.currentIndex = model.index;
          }
      }

      Row {
        id: topLayout
        x: 10; y: 10;
        height: collectionIcon.height;
        width: parent.width
        spacing: 10

        Image {
            id: collectionIcon
            pixmap: KDE.iconToPixmap( model.decoration, height );
            width: 48; height: 48
        }

        Column {
          height: collectionIcon.height
          width: background.width - collectionIcon.width - 20
          spacing: 5
          Text {
            text : model.display
            font.bold: true
          }
        }
      }
    }
  }

  Component {
    id: highlight
    Rectangle {
      color: palette.highlight
      radius: 5
    }
  }

  ListView
  {
    id: collectionListView
    anchors.fill: parent
    delegate : collectionViewDelegate
    highlight: highlight
    highlightFollowsCurrentItem: true
    focus: true
  }
}
