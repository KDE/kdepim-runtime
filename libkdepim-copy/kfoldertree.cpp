// -*- c-basic-offset: 2 -*-

#include "kfoldertree.h"
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <qpainter.h>
#include <qapplication.h>
#include <qheader.h>
#include <qstyle.h>

//-----------------------------------------------------------------------------
KFolderTreeItem::KFolderTreeItem( KFolderTree *parent, const QString & label,
				  Protocol protocol, Type type )
  : KListViewItem( parent, label ), mProtocol( protocol ), mType( type ),
    mUnread(-1), mTotal(0)
{
}

//-----------------------------------------------------------------------------
KFolderTreeItem::KFolderTreeItem( KFolderTreeItem *parent,
				  const QString & label, Protocol protocol, Type type,
          int unread, int total )
    : KListViewItem( parent, label ), mProtocol( protocol ), mType( type ),
      mUnread( unread ), mTotal( total )
{
}

//-----------------------------------------------------------------------------
int KFolderTreeItem::protocolSortingKey() const
{
  // protocol dependant sorting order:
  // local < imap < news < search < other
  switch ( mProtocol ) {
  case Local:
    return 1;
  case CachedImap:
  case Imap:
    return 2;
  case News:
    return 3;
  case Search:
    return 4;
  default:
    return 42;
  }
}

//-----------------------------------------------------------------------------
int KFolderTreeItem::typeSortingKey() const
{
  // type dependant sorting order:
  // inbox < outbox < sent-mail < trash < drafts
  // < calendar < contacts < notes < tasks
  // < normal folders
  switch ( mType ) {
  case Inbox:
    return 1;
  case Outbox:
    return 2;
  case SentMail:
    return 3;
  case Trash:
    return 4;
  case Drafts:
    return 5;
  case Calendar:
    return 6;
  case Contacts:
    return 7;
  case Notes:
    return 8;
  case Tasks:
    return 9;
  default:
    return 42;
  }
}

//-----------------------------------------------------------------------------
int KFolderTreeItem::compare( QListViewItem * i, int col, bool ) const
{
  KFolderTreeItem* other = static_cast<KFolderTreeItem*>( i );

  if (col == 0)
  {
    // sort by folder

    // local root-folder
    if ( depth() == 0 && mProtocol == NONE )
      return -1;
    if ( other->depth() == 0 && other->protocol() == NONE )
      return 1;

    // first compare by protocol
    int thisKey = protocolSortingKey();
    int thatKey = other->protocolSortingKey();
    if ( thisKey < thatKey )
      return -1;
    if ( thisKey > thatKey )
      return 1;

    // then compare by type
    thisKey = typeSortingKey();
    thatKey = other->typeSortingKey();
    if ( thisKey < thatKey )
      return -1;
    if ( thisKey > thatKey )
      return 1;

    // and finally compare by name
    return text( 0 ).localeAwareCompare( other->text( 0 ) );
  }
  else
  {
    // sort by unread or total-column
    int a = 0, b = 0;
    if (col == static_cast<KFolderTree*>(listView())->unreadIndex())
    {
      a = mUnread;
      b = other->unreadCount();
    }
    else if (col == static_cast<KFolderTree*>(listView())->totalIndex())
    {
      a = mTotal;
      b = other->totalCount();
    }

    if ( a == b )
      return 0;
    else
      return (a < b ? -1 : 1);
  }
}

//-----------------------------------------------------------------------------
void KFolderTreeItem::setUnreadCount( int aUnread )
{
  if ( aUnread < 0 ) return;

  mUnread = aUnread;

  QString unread = QString::null;
  if (mUnread == 0)
    unread = "- ";
  else {
    unread.setNum(mUnread);
    unread += " ";
  }

  setText( static_cast<KFolderTree*>(listView())->unreadIndex(),
      unread );
}

//-----------------------------------------------------------------------------
void KFolderTreeItem::setTotalCount( int aTotal )
{
  if ( aTotal < 0 ) return;

  mTotal = aTotal;

  QString total = QString::null;
  if (mTotal == 0)
    total = "- ";
  else {
    total.setNum(mTotal);
    total += " ";
  }

  setText( static_cast<KFolderTree*>(listView())->totalIndex(),
      total );
}

//-----------------------------------------------------------------------------
int KFolderTreeItem::countUnreadRecursive()
{
  int count = (mUnread > 0) ? mUnread : 0;

  for ( QListViewItem *item = firstChild() ;
      item ; item = item->nextSibling() )
  {
    count += static_cast<KFolderTreeItem*>(item)->countUnreadRecursive();
  }

  return count;
}

//-----------------------------------------------------------------------------
void KFolderTreeItem::paintCell( QPainter * p, const QColorGroup & cg,
                                  int column, int width, int align )
{
  KFolderTree *ft = static_cast<KFolderTree*>(listView());

  const int unreadRecursiveCount = countUnreadRecursive();
  const int unreadCount = ( mUnread > 0 ) ? mUnread : 0;

  /* The below is exceedingly silly, but Ingo insists that the unread
   * count that is shown in parenthesis after the folder name must
   * be configurable in color. That means that paintCell needs to do
   * two painting passes which flickers. Since that flicker is not
   * needed when there is the unread column, special case that. */
  if ( ft->isUnreadActive() ) {
    if ( (column == 0 || column == ft->unreadIndex())
          && ( unreadCount > 0 || ( !isOpen() && unreadRecursiveCount > 0 ) ) )
    {
      QFont f = p->font();
      f.setWeight(QFont::Bold);
      p->setFont(f);
    }
    KListViewItem::paintCell( p, cg, column, width, align );
  } else {
    QListView *lv = listView();
    QString oldText = text(column);

    // set an empty text so that we can have our own implementation (see further down)
    // but still benefit from KListView::paintCell
    setText( column, "" );

    KListViewItem::paintCell( p, cg, column, width, align );

    int r = lv ? lv->itemMargin() : 1;
    const QPixmap *icon = pixmap( column );
    int marg = lv ? lv->itemMargin() : 1;

    QString t;
    QRect br;
    setText( column, oldText );
    if ( isSelected() )
      p->setPen( cg.highlightedText() );
    else
      p->setPen( ft->paintInfo().colFore );

    if ( icon ) {
      r += icon->width() + lv->itemMargin();
    }
    t = text( column );
    if ( !t.isEmpty() )
    {
      // use a bold-font for the folder- and the unread-columns
      if ( (column == 0 || column == ft->unreadIndex())
            && ( unreadCount > 0
                 || ( !isOpen() && unreadRecursiveCount > 0 ) ) )
      {
        QFont f = p->font();
        f.setWeight(QFont::Bold);
        p->setFont(f);
      }
      p->drawText( r, 0, width-marg-r, height(),
          align | AlignVCenter, t, -1, &br );
      if (!isSelected())
        p->setPen( ft->paintInfo().colUnread );
      if (column == 0) {
        // draw the unread-count if the unread-column is not active
        QString unread;

        if ( !ft->isUnreadActive()
             && ( unreadCount > 0
                  || ( !isOpen() && unreadRecursiveCount > 0 ) ) ) {
          if ( isOpen() )
            unread = " (" + QString::number( unreadCount ) + ")";
          else if ( unreadRecursiveCount == unreadCount || mType == Root )
            unread = " (" + QString::number( unreadRecursiveCount ) + ")";
          else
            unread = " (" + QString::number( unreadCount ) + " + " +
                     QString::number( unreadRecursiveCount-unreadCount ) + ")";

          p->drawText( br.right(), 0, width-marg-br.right(), height(),
                       align | AlignVCenter, unread );
        }
      }
    } // end !t.isEmpty()
  }
}


//=============================================================================


KFolderTree::KFolderTree( QWidget *parent, const char* name )
  : KListView( parent, name ), mUnreadIndex(-1), mTotalIndex(-1)
{
  // GUI-options
  setStyleDependantFrameWidth();
  setAcceptDrops(true);
  setDropVisualizer(false);
  setAllColumnsShowFocus(true);
  setShowSortIndicator(true);
  setUpdatesEnabled(true);
  setItemsRenameable(false);
  setRootIsDecorated(true);
  setSelectionModeExt(Extended);
  setAlternateBackground(QColor());
  setFullWidth(true);
  disableAutoSelection();
}

//-----------------------------------------------------------------------------
void KFolderTree::setStyleDependantFrameWidth()
{
  // set the width of the frame to a reasonable value for the current GUI style
  int frameWidth;
  if( style().isA("KeramikStyle") )
    frameWidth = style().pixelMetric( QStyle::PM_DefaultFrameWidth ) - 1;
  else
    frameWidth = style().pixelMetric( QStyle::PM_DefaultFrameWidth );
  if ( frameWidth < 0 )
    frameWidth = 0;
  if ( frameWidth != lineWidth() )
    setLineWidth( frameWidth );
}

//-----------------------------------------------------------------------------
void KFolderTree::styleChange( QStyle& oldStyle )
{
  setStyleDependantFrameWidth();
  KListView::styleChange( oldStyle );
}

//-----------------------------------------------------------------------------
void KFolderTree::drawContentsOffset( QPainter * p, int ox, int oy,
                                       int cx, int cy, int cw, int ch )
{
  bool oldUpdatesEnabled = isUpdatesEnabled();
  setUpdatesEnabled(false);
  KListView::drawContentsOffset( p, ox, oy, cx, cy, cw, ch );
  setUpdatesEnabled(oldUpdatesEnabled);
}

//-----------------------------------------------------------------------------
void KFolderTree::contentsMousePressEvent( QMouseEvent *e )
{
    setSelectionModeExt(Single);
    KListView::contentsMousePressEvent(e);
}

//-----------------------------------------------------------------------------
void KFolderTree::contentsMouseReleaseEvent( QMouseEvent *e )
{
    KListView::contentsMouseReleaseEvent(e);
    setSelectionModeExt(Extended);
}

//-----------------------------------------------------------------------------
void KFolderTree::addAcceptableDropMimetype( const char *mimeType, bool outsideOk )
{
  int oldSize = mAcceptableDropMimetypes.size();
  mAcceptableDropMimetypes.resize(oldSize+1);
  mAcceptOutside.resize(oldSize+1);

  mAcceptableDropMimetypes.at(oldSize) =  mimeType;
  mAcceptOutside.setBit(oldSize, outsideOk);
}

//-----------------------------------------------------------------------------
bool KFolderTree::acceptDrag( QDropEvent* event ) const
{
  QListViewItem* item = itemAt(contentsToViewport(event->pos()));

  for (uint i = 0; i < mAcceptableDropMimetypes.size(); i++)
  {
    if (event->provides(mAcceptableDropMimetypes[i]))
    {
      if (item)
        return (static_cast<KFolderTreeItem*>(item))->acceptDrag(event);
      else
        return mAcceptOutside[i];
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void KFolderTree::addUnreadColumn( const QString & name, int width )
{
  mUnreadIndex = addColumn( name, width );
  setColumnAlignment( mUnreadIndex, qApp->reverseLayout() ? Qt::AlignLeft : Qt::AlignRight );
  header()->adjustHeaderSize();
}

//-----------------------------------------------------------------------------
void KFolderTree::addTotalColumn( const QString & name, int width )
{
  mTotalIndex = addColumn( name, width );
  setColumnAlignment( mTotalIndex, qApp->reverseLayout() ? Qt::AlignLeft : Qt::AlignRight );
  header()->adjustHeaderSize();
}

//-----------------------------------------------------------------------------
void KFolderTree::removeUnreadColumn()
{
  if ( !isUnreadActive() ) return;
  removeColumn( mUnreadIndex );
  if ( isTotalActive() && mTotalIndex > mUnreadIndex )
    mTotalIndex--;
  mUnreadIndex = -1;
  header()->adjustHeaderSize();
}

//-----------------------------------------------------------------------------
void KFolderTree::removeTotalColumn()
{
  if ( !isTotalActive() ) return;
  removeColumn( mTotalIndex );
  if ( isUnreadActive() && mTotalIndex < mUnreadIndex )
    mUnreadIndex--;
  mTotalIndex = -1;
  header()->adjustHeaderSize();
}

//-----------------------------------------------------------------------------
void KFolderTree::setFullWidth( bool fullWidth )
{
  if (fullWidth)
    header()->setStretchEnabled( true, 0 );
}

#include "kfoldertree.moc"
