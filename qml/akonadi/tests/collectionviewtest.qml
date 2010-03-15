import Qt 4.6
import org.kde 4.5
import org.kde.akonadi 4.5

Rectangle {
  color: white
  height: 480
  width: 800

  CollectionView {
    id: listView
    anchors.fill: parent
    model: collectionModel
  }
  Binding {
    target: application
    property: "collectionRow"
    value: listView.currentIndex
  }
}
