/******************************************************************************
 *
 * This file is part of libkdepim.
 *
 * Copyright (C) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 *****************************************************************************/

#include "foldertreewidget.h"

#include <kio/global.h> // for KIO::filesize_t and related functions
#include <klocale.h>

#include <QHeaderView>
#include <QDropEvent>
#include <QMimeData>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QPainter>
#include <QFontMetrics>

#define FOLDERTREEWIDGETITEM_TYPE ( QTreeWidgetItem::UserType + 0xcafe )


namespace KPIM {

#define ITEM_LABEL_RIGHT_MARGIN 2
#define ITEM_LABEL_TO_UNREADCOUNT_SPACING 2

class FolderTreeWidgetItemLabelColumnDelegate : public QStyledItemDelegate
{
protected:
  FolderTreeWidget *mFolderTreeWidget;

public:
  FolderTreeWidgetItemLabelColumnDelegate( FolderTreeWidget *parent )
    : QStyledItemDelegate( parent ), mFolderTreeWidget( parent ) {};

  ~FolderTreeWidgetItemLabelColumnDelegate()
  {};

  virtual QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
  {
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    opt.text = "X"; // fake a text (so the height is computed correctly)
    opt.features |= QStyleOptionViewItemV2::HasDisplay;

    return mFolderTreeWidget->style()->sizeFromContents( QStyle::CT_ItemViewItem, &opt, QSize(), mFolderTreeWidget );
  }

  virtual void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
  {
    //
    // The equilibrism in this function attempts to rely on QStyle for most of the job.
    //
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    opt.text = ""; // draw no text for me, please.. I'll do it in a while

    QStyle * style = mFolderTreeWidget->style();

    style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, mFolderTreeWidget );

    // FIXME: this is plain ugly :D
    // OTOH seaching the item by model index is extremely expensive when compared to a cast.
    // We're also using static_cast<> instead of dynamic_cast<> to avoid performance loss:
    // we actually trust the developer to add only real FolderTreeWidgetItems to this widget.
    FolderTreeWidgetItem * item = static_cast<FolderTreeWidgetItem *>( index.internalPointer() );
    if ( !item )
      return; // ugh :/

    // FIXME: opt->direction == Qt::RightToLeft ?

    // draw the labelText().
    QRect textRect = style->subElementRect( QStyle::SE_ItemViewItemText, &opt, mFolderTreeWidget );
    textRect.setWidth( textRect.width() - ITEM_LABEL_RIGHT_MARGIN );

    // keeping indipendent state variables is faster than saving and restoring the whole painter state
    QPen oldPen = painter->pen();
    QFont oldFont = painter->font();

    if ( item->isCloseToQuota() && ( !( opt.state & QStyle::State_Selected ) ) )
    {
      painter->setPen( QPen( mFolderTreeWidget->closeToQuotaWarningColor(), 0 ) );
    } else {
      QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;

      if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

      QPalette::ColorRole cr = ( opt.state & QStyle::State_Selected ) ? QPalette::HighlightedText : QPalette::Text;
      painter->setPen( QPen( opt.palette.brush( cg, cr ), 0 ) );
    }

    int unread = item->unreadCount();
    int childUnread = item->childrenUnreadCount();
    bool displayChildUnread = ( childUnread > 0 ) && ( !item->isExpanded() );

    if ( ( unread > 0 ) || displayChildUnread )
    {
      // font is bold for sure (unread stuff in folder)
      QFont f = opt.font;
      f.setBold( true );
      painter->setFont( f );

      QFontMetrics fm( painter->font() ); // use the bold font metrics

      if (
          ( mFolderTreeWidget->unreadColumnIndex() == -1 ) ||
          mFolderTreeWidget->isColumnHidden( mFolderTreeWidget->unreadColumnIndex() )
        )
      {
        // we need to paint the unread count too
        QString unreadText;

        if ( unread > 0 )
        {
          if ( displayChildUnread )
            unreadText = QString("(%1 + %2)").arg( unread ).arg( childUnread );
          else 
            unreadText = QString("(%1)").arg( unread );
        } else
          unreadText = QString("(0 + %1)").arg( childUnread );

        // compute the potential width so we know if elision has to be performed

        int unreadWidth = fm.width( unreadText );
        int labelWidth = fm.width( item->labelText() );
        int maxWidth = labelWidth + ITEM_LABEL_TO_UNREADCOUNT_SPACING + unreadWidth;

        if ( maxWidth > textRect.width() ) 
        {
          // must elide
          QString label = fm.elidedText( item->labelText(), Qt::ElideRight , textRect.width() - ( ITEM_LABEL_TO_UNREADCOUNT_SPACING + unreadWidth ) );

          // the condition inside this call is an optimisation (it's faster than simply label != item->labelText())
          item->setLabelTextElided( ( label.length() != item->labelText().length() ) || ( label != item->labelText() ) );

          painter->drawText( textRect, Qt::AlignLeft | Qt::TextSingleLine | Qt::AlignVCenter, label );

          if ( !( opt.state & QStyle::State_Selected ) )
            painter->setPen( QPen( mFolderTreeWidget->unreadCountColor(), 0 ) );
          painter->drawText( textRect, Qt::AlignRight | Qt::TextSingleLine | Qt::AlignVCenter, unreadText );

        } else {
          // no elision needed
          item->setLabelTextElided( false );

          painter->drawText( textRect, Qt::AlignLeft | Qt::TextSingleLine | Qt::AlignVCenter, item->labelText() );

          textRect.setLeft( textRect.left() + labelWidth + ITEM_LABEL_TO_UNREADCOUNT_SPACING );

          if ( !( opt.state & QStyle::State_Selected ) )
            painter->setPen( QPen( mFolderTreeWidget->unreadCountColor(), 0 ) );
          painter->drawText( textRect, Qt::AlignLeft | Qt::TextSingleLine | Qt::AlignVCenter, unreadText );
        }
      } else {
        // got unread messages: bold font but no special text tricks
        QString label = fm.elidedText( item->labelText(), Qt::ElideRight , textRect.width() );

        // the condition inside this call is an optimisation (it's faster than simply label != item->labelText())
        item->setLabelTextElided( ( label.length() != item->labelText().length() ) || ( label != item->labelText() ) );

        painter->drawText( textRect, Qt::AlignLeft | Qt::TextSingleLine | Qt::AlignVCenter, label );
      }
    } else {
      // no unread messages: normal font, no text tricks
      QString label = opt.fontMetrics.elidedText( item->labelText(), Qt::ElideRight , textRect.width() );

      // the condition inside this call is an optimisation (it's faster than simply label != item->labelText())
      item->setLabelTextElided( ( label.length() != item->labelText().length() ) || ( label != item->labelText() ) );

      painter->setFont( opt.font );
      painter->drawText( textRect, Qt::AlignLeft | Qt::TextSingleLine | Qt::AlignVCenter, label );
    }

    painter->setPen( oldPen );
    painter->setFont( oldFont );
  }
};




FolderTreeWidget::FolderTreeWidget( QWidget *parent , const char *name )
: KPIM::TreeWidget( parent , name ),
  mUnreadCountColor( Qt::blue ), mCloseToQuotaWarningColor( Qt::red )
{
  mLabelColumnIndex = -1;
  mUnreadColumnIndex = -1;
  mTotalColumnIndex = -1;
  mDataSizeColumnIndex = -1;

  setAlternatingRowColors( true );
  setAcceptDrops( true );
  setAllColumnsShowFocus( true );
  setRootIsDecorated( true );
  setSortingEnabled( true );
  setUniformRowHeights( true ); // optimize please
  //setAnimated( true ); // this slows down everything a lot

  header()->setSortIndicatorShown( true );
  header()->setClickable( true );
  //setSelectionMode( Extended );

  connect( this, SIGNAL( itemExpanded( QTreeWidgetItem * ) ),
           SLOT( updateExpandedState( QTreeWidgetItem * ) ) );
  connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
           SLOT( updateExpandedState( QTreeWidgetItem * ) ) );

}

int FolderTreeWidget::addLabelColumn( const QString &headerLabel )
{
  if ( mLabelColumnIndex == -1 )
  {
    mLabelColumnIndex = addColumn( headerLabel );
    setItemDelegateForColumn( mLabelColumnIndex, new FolderTreeWidgetItemLabelColumnDelegate( this ) );
  }
  return mLabelColumnIndex;
}

bool FolderTreeWidget::labelColumnVisible() const
{
  if ( mLabelColumnIndex == -1 )
    return false;
  return !isColumnHidden( mLabelColumnIndex );
}

int FolderTreeWidget::addTotalColumn( const QString &headerLabel )
{
  if ( mTotalColumnIndex == -1 )
    mTotalColumnIndex = addColumn( headerLabel , Qt::AlignRight );
  return mTotalColumnIndex;
}

bool FolderTreeWidget::totalColumnVisible() const
{
  if ( mTotalColumnIndex == -1 )
    return false;
  return !isColumnHidden( mTotalColumnIndex );
}

int FolderTreeWidget::addUnreadColumn( const QString &headerLabel )
{
  if ( mUnreadColumnIndex == -1 )
    mUnreadColumnIndex = addColumn( headerLabel , Qt::AlignRight );
  return mUnreadColumnIndex;
}

bool FolderTreeWidget::unreadColumnVisible() const
{
  if ( mUnreadColumnIndex == -1 )
    return false;
  return !isColumnHidden( mUnreadColumnIndex );
}

int FolderTreeWidget::addDataSizeColumn( const QString &headerLabel )
{
  if ( mDataSizeColumnIndex == -1 )
    mDataSizeColumnIndex = addColumn( headerLabel , Qt::AlignRight );
  return mDataSizeColumnIndex;
}

bool FolderTreeWidget::dataSizeColumnVisible() const
{
  if ( mDataSizeColumnIndex == -1 )
    return false;
  return !isColumnHidden( mDataSizeColumnIndex );
}

void FolderTreeWidget::updateColumnForItem( FolderTreeWidgetItem * item, int columnIndex )
{
  update( indexFromItem( item, columnIndex ) );
}

void FolderTreeWidget::updateExpandedState( QTreeWidgetItem *item )
{
  FolderTreeWidgetItem *folderItem = dynamic_cast<FolderTreeWidgetItem*>( item );
  Q_ASSERT( folderItem );
  if ( folderItem )
    folderItem->updateExpandedState();
}

void FolderTreeWidget::setCloseToQuotaWarningColor( const QColor &clr )
{
  mCloseToQuotaWarningColor = clr;
  update();
}

void FolderTreeWidget::setUnreadCountColor( const QColor &clr )
{
  mUnreadCountColor = clr;
  update();
}



FolderTreeWidgetItem::FolderTreeWidgetItem(
        FolderTreeWidget *parent,
        const QString &label,
        Protocol protocol,
        FolderType folderType
    ) : QTreeWidgetItem( parent, FOLDERTREEWIDGETITEM_TYPE ),
        mProtocol( protocol ), mFolderType( folderType ), mLabelText( label ),
        mTotalCount( 0 ), mUnreadCount( 0 ), mDataSize( -1 ), mIsCloseToQuota( 0 ),
        mLabelTextElided( 0 ), mChildrenTotalCount( 0 ), mChildrenUnreadCount( 0 ),
        mChildrenDataSize( -1 ), mAlwaysDisplayCounts( false )
{
  setText( parent->labelColumnIndex(), label );
}

FolderTreeWidgetItem::FolderTreeWidgetItem(
        FolderTreeWidgetItem *parent,
        const QString &label,
        Protocol protocol,
        FolderType folderType
    ) : QTreeWidgetItem( parent, FOLDERTREEWIDGETITEM_TYPE ),
        mProtocol( protocol ), mFolderType( folderType ), mLabelText( label ),
        mTotalCount( 0 ), mUnreadCount( 0 ), mDataSize( -1 ), mIsCloseToQuota( 0 ),
        mLabelTextElided( 0 ), mChildrenTotalCount( 0 ), mChildrenUnreadCount( 0 ),
        mChildrenDataSize( -1 ), mAlwaysDisplayCounts( false )
{
  FolderTreeWidget * tree = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  if ( tree )
    setText( tree->labelColumnIndex(), label );
}

QString FolderTreeWidgetItem::protocolDescription() const
{
  switch( mProtocol )
  {
    case Local:
      return i18n( "Local" );
    break;
    case Imap:
      return i18n( "IMAP" );
    break;
    case CachedImap:
      return i18n( "Cached IMAP" );
    break;
    case News:
      return i18n( "News" );
    break;
    case Search:
      return i18n( "Search" );
    break;
    case NONE:
      return i18n( "None" );
    break;
    default:
    break;
  }

  return i18n( "Unknown" );
}

bool FolderTreeWidgetItem::updateChildrenCounts()
{
  int cc = childCount();

  if ( cc < 1 )
    return false;

  int idx = 0;


  int oldTotal = mChildrenTotalCount;
  int oldUnread = mChildrenUnreadCount;
  qint64 oldSize = mChildrenDataSize;

  mChildrenTotalCount = 0;
  mChildrenUnreadCount = 0;
  mChildrenDataSize = 0;

  bool gotValidDataSize = false;  

  while ( idx < cc )
  {
    FolderTreeWidgetItem *it = static_cast< FolderTreeWidgetItem * >( QTreeWidgetItem::child( idx ) );
    mChildrenTotalCount += it->totalCount() + it->childrenTotalCount();
    mChildrenUnreadCount += it->unreadCount() + it->childrenUnreadCount();
    if ( it->dataSize() >= 0)
    {
      gotValidDataSize = true;
      mChildrenDataSize += it->dataSize();
    }
    if ( it->childrenDataSize() >= 0 )
    {
      gotValidDataSize = true;
      mChildrenDataSize += it->childrenDataSize();
    }
    idx++;
  }

  if ( !gotValidDataSize )
    mChildrenDataSize = -1; // keep it invald

  return ( oldTotal != mChildrenTotalCount ) ||
           ( oldUnread != mChildrenUnreadCount ) ||
           ( oldSize != mChildrenDataSize );
}

void FolderTreeWidgetItem::setLabelText( const QString &label )
{
  mLabelText = label;
  // We set the text of the item so QStyle will compute the height correctly
  FolderTreeWidget * tree = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  int idx = tree->labelColumnIndex();
  if ( tree && idx >= 0 )
  {
    setText( idx, label );  
    setTextAlignment( idx, Qt::AlignRight );
  }
}

void FolderTreeWidgetItem::setUnreadCount( int unreadCount )
{
  mUnreadCount = unreadCount;
  int unreadCountToDisplay = unreadCount;

  if ( mChildrenUnreadCount > 0 && !isExpanded() )
    unreadCountToDisplay += mChildrenUnreadCount;

  FolderTreeWidget * tree = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  int idx = tree->unreadColumnIndex();
  // FIXME: Why the "parent()" logic is hardwired here ?
  if ( tree && idx >= 0 && ( parent() || mAlwaysDisplayCounts ) )
  {
    if ( !isExpanded() )
      setText( idx, QString::number( unreadCountToDisplay ) );
    else
      setText( idx, QString() );
    setTextAlignment( idx, Qt::AlignRight );
  }
}

void FolderTreeWidgetItem::setTotalCount( int totalCount )
{
  mTotalCount = totalCount;
  int totalCountToDisplay = totalCount;

  if ( mChildrenTotalCount > 0 && !isExpanded() )
    totalCountToDisplay += mChildrenTotalCount;

  FolderTreeWidget * tree = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  int idx = tree->totalColumnIndex();
  if ( tree && idx >= 0 )
  {
    // FIXME: Why the "parent()" logic is hardwired here ?
    if ( parent() || mAlwaysDisplayCounts || !isExpanded() )
      setText( idx, QString::number( totalCountToDisplay ) );
    else
      setText( idx, QString() );
    setTextAlignment( idx, Qt::AlignRight );
  }
}

void FolderTreeWidgetItem::setDataSize( qint64 dataSize )
{
  mDataSize = dataSize;

  QString txt;
  QString sizeText = KIO::convertSize( mDataSize >= 0 ? (KIO::filesize_t)mDataSize : (KIO::filesize_t)0 );
  QString childSizeText = KIO::convertSize( (KIO::filesize_t)mChildrenDataSize );

  // A top level item, they all have size 0
  // FIXME: Why this logic is hardwired here ?
  if ( !parent() && !mAlwaysDisplayCounts ) {
    if ( mChildrenDataSize >= 0 && !isExpanded() )
      txt = childSizeText;
    else
      txt = QString();
  }

  // Not a top level item (or always displays counts)
  else if ( ( mDataSize >= 0 ) || ( mChildrenDataSize >= 0 ) ) {
   txt = sizeText;
   if ( !isExpanded() ) {
      if ( mChildrenDataSize >= 0 )
        txt += " + " + childSizeText;
    }
  }
  else
    txt = "-";

  FolderTreeWidget * tree = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  int idx = tree->dataSizeColumnIndex();
  if ( tree && idx >= 0 )
  {
    setText( idx, txt );
    setTextAlignment( idx, Qt::AlignRight );
  }
}

void FolderTreeWidgetItem::updateExpandedState()
{
  // Just trigger an update of the column text, it will all be handled thaere.
  setDataSize( mDataSize );
  setTotalCount( mTotalCount );
  setUnreadCount( mUnreadCount );
}

void FolderTreeWidgetItem::updateColumn( int columnIndex )
{
  FolderTreeWidget * tree = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  if ( tree )
    tree->updateColumnForItem( this, columnIndex );
}

void FolderTreeWidgetItem::setIsCloseToQuota( bool closeToQuota )
{
  if ( ( mIsCloseToQuota == 1 ) == closeToQuota )
    return;
  mIsCloseToQuota = closeToQuota ? 1 : 0;

  FolderTreeWidget * tree = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  if ( tree && tree->labelColumnVisible() )
    updateColumn( tree->labelColumnIndex() );
}


bool FolderTreeWidgetItem::operator < ( const QTreeWidgetItem &other ) const
{
  // FIXME: Sort by children counts too ?

  int sortCol = treeWidget()->sortColumn();
  if ( sortCol < 0 )
     return true; // just "yes" :D

  FolderTreeWidget * w = dynamic_cast< FolderTreeWidget * >( treeWidget() );
  if ( w )
  {
    // sanity check
    if ( dynamic_cast<const FolderTreeWidgetItem*>( &other ) )
    {
      const FolderTreeWidgetItem * oitem = dynamic_cast< const FolderTreeWidgetItem * >( &other );
      if ( oitem )
      {
        if ( sortCol == w->unreadColumnIndex() )
          return mUnreadCount < oitem->unreadCount();
        if ( sortCol == w->totalColumnIndex() )
          return mTotalCount < oitem->totalCount();
        if ( sortCol == w->dataSizeColumnIndex() )
          return mDataSize < oitem->dataSize();
        if ( sortCol == w->labelColumnIndex() )
        {
          // Special sorting based on the item type
          int thisProto = ( int ) mProtocol;
          int thatProto = ( int ) oitem->protocol();
          if ( thisProto < thatProto )
            return true;
          if ( thisProto > thatProto )
            return false;

          int thisType = ( int ) mFolderType;
          int thatType = ( int ) oitem->folderType();
          if ( thisType < thatType )
            return true;
          if ( thisType > thatType )
            return false;

          // and finally compare by name
          return mLabelText.toLower() < oitem->labelText().toLower();
        }
      }
    }
  }

  return text(sortCol) < other.text(sortCol);
}

}
