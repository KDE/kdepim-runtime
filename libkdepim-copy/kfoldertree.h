/* -*- mode: C++; c-file-style: "gnu" -*-

   This file is part of the KDE libraries
   Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>
   Copyright (C) 2002 Marc Mutz <mutz@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __KFOLDERTREE
#define __KFOLDERTREE

#include <qpixmap.h>
#include <qbitarray.h>
#include <qdragobject.h>
#include <klistview.h>
#include <kdepimmacros.h>

class KFolderTree;

/** Information shared by all items in a list view */
struct KPaintInfo {

  // Popup ids for toggle-able columns
  enum ColumnIds
  {
    COL_SIZE,
    COL_ATTACHMENT,
    COL_IMPORTANT,
    COL_SPAM_HAM,
    COL_WATCHED_IGNORED,
    COL_STATUS,
    COL_SIGNED,
    COL_CRYPTO,
    COL_RECEIVER,
    COL_SCORE
  };

  KPaintInfo() :
    pixmapOn(false),

    showSize(false),
    showAttachment(false),
    showImportant(false),
    showSpamHam(false),
    showWatchedIgnored(false),
    showStatus(false),
    showSigned(false),
    showCrypto(false),
    showReceiver(false),
    showScore(false),

    scoreCol(-1),
    flagCol(-1),
    senderCol(-1),
    receiverCol(-1),
    subCol(-1),
    dateCol(-1),
    sizeCol(-1),
    attachmentCol(-1),
    importantCol(-1),
    spamHamCol(-1),
    watchedIgnoredCol(-1),
    statusCol(-1),
    signedCol(-1),
    cryptoCol(-1),

    orderOfArrival(false),
    status(false),
    showCryptoIcons(false),
    showAttachmentIcon(false)
    {}

  bool pixmapOn;
  QPixmap pixmap;
  QColor colFore;
  QColor colBack;
  QColor colNew;
  QColor colUnread;
  QColor colFlag;

  bool showSize;
  bool showAttachment;
  bool showImportant;
  bool showSpamHam;
  bool showWatchedIgnored;
  bool showStatus;
  bool showSigned;
  bool showCrypto;
  bool showReceiver;
  bool showScore;

  int scoreCol;
  int flagCol;
  int senderCol;
  int receiverCol;
  int subCol;
  int dateCol;
  int sizeCol;
  int attachmentCol;
  int importantCol;
  int spamHamCol;
  int watchedIgnoredCol;
  int statusCol;
  int signedCol;
  int cryptoCol;

  bool orderOfArrival;
  bool status;
  bool showCryptoIcons;
  bool showAttachmentIcon;
};

//==========================================================================

class KDE_EXPORT KFolderTreeItem : public KListViewItem
{
  public:
    /** Protocol information */
    enum Protocol {
      Imap,
      Local,
      News,
      CachedImap,
      Search,
      NONE
    };

    /** Type information */
    enum Type {
      Inbox,
      Outbox,
      SentMail,
      Trash,
      Drafts,
      Templates,
      Root,
      Calendar,
      Tasks,
      Journals,
      Contacts,
      Notes,
      Other
    };

    /** constructs a root-item */
    KFolderTreeItem( KFolderTree *parent, const QString & label=QString::null,
        Protocol protocol=NONE, Type type=Root );

    /** constructs a child-item */
    KFolderTreeItem( KFolderTreeItem *parent, const QString & label=QString::null,
        Protocol protocol=NONE, Type type=Other, int unread=0, int total=0 );

    /** compare */
    virtual int compare( QListViewItem * i, int col,
        bool ascending ) const;

    /** set/get the unread-count */
    int unreadCount() { return mUnread; }
    virtual void setUnreadCount( int aUnread );

    /** set/get the total-count */
    int totalCount() { return mTotal; }
    virtual void setTotalCount( int aTotal );

    /** set/get the protocol of the item */
    Protocol protocol() const { return mProtocol; }
    virtual void setProtocol( Protocol aProtocol ) { mProtocol = aProtocol; }

    /** set/get the type of the item */
    Type type() const { return mType; }
    virtual void setType( Type aType ) { mType = aType; }

    /** recursive unread count */
    virtual int countUnreadRecursive();

    /** paints the cell */
    virtual void paintCell( QPainter * p, const QColorGroup & cg,
        int column, int width, int align );

    /** dnd */
    virtual bool acceptDrag(QDropEvent* ) const { return true; }

  private:
    /** returns a sorting key based on the folder's protocol */
    int protocolSortingKey() const;
    /** returns a sorting key based on the folder's type */
    int typeSortingKey() const;

  protected:
    /** reimplement to use special squeezing algorithm for the folder name */
    virtual QString squeezeFolderName( const QString &text,
                                       const QFontMetrics &fm,
                                       uint width ) const;

    Protocol mProtocol;
    Type mType;
    int mUnread;
    int mTotal;
};

//==========================================================================

class KDE_EXPORT KFolderTree : public KListView
{
  Q_OBJECT

  public:
    KFolderTree( QWidget *parent, const char *name=0 );

    /** registers MIMETypes that are handled
      @param mimeType the name of the MIMEType
      @param outsideOk accept drops of this type even if
      the mouse cursor is not on top of an item */
    virtual void addAcceptableDropMimetype( const char *mimeType, bool outsideOk );

    /** checks if the drag is acceptable */
    virtual bool acceptDrag( QDropEvent* event ) const;

    /** returns the KPaintInfo */
    KPaintInfo paintInfo() const { return mPaintInfo; }

    /** add/remove unread/total-columns */
    virtual void addUnreadColumn( const QString & name, int width=70 );
    virtual void removeUnreadColumn();
    virtual void addTotalColumn( const QString & name, int width=70 );
    virtual void removeTotalColumn();

    /** the current index of the unread/total column */
    int unreadIndex() const { return mUnreadIndex; }
    int totalIndex() const { return mTotalIndex;  }

    /** is the unread/total-column active? */
    bool isUnreadActive() const { return mUnreadIndex >= 0; }
    bool isTotalActive() const { return mTotalIndex >=  0; }

    /** reimp to set full width of the _first_ column */
    virtual void setFullWidth( bool fullWidth );

  protected:
    /** reimplemented in order to update the frame width in case of a changed
        GUI style */
    void styleChange( QStyle& oldStyle );

    /** Set the width of the frame to a reasonable value for the current GUI
        style */
    void setStyleDependantFrameWidth();

    virtual void drawContentsOffset( QPainter * p, int ox, int oy,
        int cx, int cy, int cw, int ch );

    virtual void contentsMousePressEvent( QMouseEvent *e );
    virtual void contentsMouseReleaseEvent( QMouseEvent *e );

    /** for mimetypes */
    QMemArray<const char*> mAcceptableDropMimetypes;
    QBitArray mAcceptOutside;

    /** shared information */ // ### why isn't it then static? ;-)
    KPaintInfo mPaintInfo;

    /** current index of unread/total-column
     * -1 is deactivated */
    int mUnreadIndex;
    int mTotalIndex;

  private slots:
    /** repaints the complete column (instead of only parts of it as done in
        QListView) if the size has changed */
    void slotSizeChanged( int section, int oldSize, int newSize );

};

#endif
