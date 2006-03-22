/*
    ksubscription.cpp

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "ksubscription.h"
#include "kaccount.h"

#include <qlayout.h>
#include <qtimer.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <q3header.h>
#include <qtoolbutton.h>
//Added by qt3to4:
#include <QGridLayout>
#include <Q3PtrList>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <kseparator.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>


//=============================================================================

KGroupInfo::KGroupInfo(const QString &name, const QString &description,
    bool newGroup, bool subscribed,
    Status status, QString path)
  : name(name), description(description),
    newGroup(newGroup), subscribed(subscribed),
    status(status), path(path)
{
}

//-----------------------------------------------------------------------------
bool KGroupInfo::operator== (const KGroupInfo &gi2)
{
  return (name == gi2.name);
}

//-----------------------------------------------------------------------------
bool KGroupInfo::operator< (const KGroupInfo &gi2)
{
  return (name < gi2.name);
}

//=============================================================================

GroupItem::GroupItem( Q3ListView *v, const KGroupInfo &gi, KSubscription* browser,
    bool isCheckItem )
  : Q3CheckListItem( v, gi.name, isCheckItem ? CheckBox : CheckBoxController ),
    mInfo( gi ), mBrowser( browser ), mIsCheckItem( isCheckItem ),
    mIgnoreStateChange( false )
{
  if (listView()->columns() > 1)
    setDescription();
}

//-----------------------------------------------------------------------------
GroupItem::GroupItem( Q3ListViewItem *i, const KGroupInfo &gi, KSubscription* browser,
    bool isCheckItem )
  : Q3CheckListItem( i, gi.name, isCheckItem ? CheckBox : CheckBoxController ),
    mInfo( gi ), mBrowser( browser ), mIsCheckItem( isCheckItem ),
    mIgnoreStateChange( false )
{
  if (listView()->columns() > 1)
    setDescription();
}

//-----------------------------------------------------------------------------
void GroupItem::setInfo( KGroupInfo info )
{
  mInfo = info;
  setText(0, mInfo.name);
  if (listView()->columns() > 1)
    setDescription();
}

//-----------------------------------------------------------------------------
void GroupItem::setDescription()
{
  setText(1, mInfo.description);
}

//-----------------------------------------------------------------------------
void GroupItem::setOn( bool on )
{
  if (mBrowser->isLoading())
  {
    // set this only if we're loading/creating items
    // otherwise changes are only permanent when the dialog is saved
    mInfo.subscribed = on;
  }
  if (isCheckItem())
    Q3CheckListItem::setOn(on);
}

//------------------------------------------------------------------------------
void GroupItem::stateChange( bool on )
{
  // delegate to parent
  if ( !mIgnoreStateChange )
    mBrowser->changeItemState(this, on);
}

//------------------------------------------------------------------------------
void GroupItem::setVisible( bool b )
{
  if (b)
  {
    Q3ListViewItem::setVisible(b);
    setEnabled(true);
  }
  else
  {
    if (isCheckItem())
    {
      bool setInvisible = true;
      for (Q3ListViewItem * lvchild = firstChild(); lvchild != 0;
          lvchild = lvchild->nextSibling())
      {
        if (lvchild->isVisible()) // item has a visible child
          setInvisible = false;
      }
      if (setInvisible)
        Q3ListViewItem::setVisible(b);
      else
      {
        // leave it visible so that children remain visible
        setOpen(true);
        setEnabled(false);
      }
    }
    else
    {
      // non-checkable item
      Q3PtrList<Q3ListViewItem> moveItems;

      for (Q3ListViewItem * lvchild = firstChild(); lvchild != 0;
          lvchild = lvchild->nextSibling())
      {
        if (static_cast<GroupItem*>(lvchild)->isCheckItem())
        {
          // remember the items
          moveItems.append(lvchild);
        }
      }
      Q3PtrListIterator<Q3ListViewItem> it( moveItems );
      for ( ; it.current(); ++it)
      {
        // move the checkitem to top
        Q3ListViewItem* parent = it.current()->parent();
        if (parent) parent->takeItem(it.current());
        listView()->insertItem(it.current());
      }
      Q3ListViewItem::setVisible(false);
    }
  }
}

//-----------------------------------------------------------------------------
void GroupItem::paintCell( QPainter * p, const QColorGroup & cg,
    int column, int width, int align )
{
  if (mIsCheckItem)
    return Q3CheckListItem::paintCell( p, cg, column, width, align );
  else
    return Q3ListViewItem::paintCell( p, cg, column, width, align );
}

//-----------------------------------------------------------------------------
void GroupItem::paintFocus( QPainter * p, const QColorGroup & cg,
    const QRect & r )
{
  if (mIsCheckItem)
    Q3CheckListItem::paintFocus(p, cg, r);
  else
    Q3ListViewItem::paintFocus(p, cg, r);
}

//-----------------------------------------------------------------------------
int GroupItem::width( const QFontMetrics& fm, const Q3ListView* lv, int column) const
{
  if (mIsCheckItem)
    return Q3CheckListItem::width(fm, lv, column);
  else
    return Q3ListViewItem::width(fm, lv, column);
}

//-----------------------------------------------------------------------------
void GroupItem::setup()
{
  if (mIsCheckItem)
    Q3CheckListItem::setup();
  else
    Q3ListViewItem::setup();
}


//=============================================================================

KSubscription::KSubscription( QWidget *parent, const QString &caption,
    KAccount * acct, int buttons, const QString &user1, bool descriptionColumn )
  : KDialogBase( parent, 0, true, caption, buttons | Help | Ok | Cancel, Ok,
      true, i18n("Reload &List"), user1 ),
    mAcct( acct )
{
  mLoading = true;
  setAttribute( Qt::WA_DeleteOnClose );

  // create Widgets
  page = new QWidget(this);
  setMainWidget(page);

  QLabel *comment = new QLabel("<p>"+
          i18n("Manage which mail folders you want to see in your folder view") + "</p>", page);

  QToolButton *clearButton = new QToolButton( page );
  clearButton->setIconSet( KGlobal::iconLoader()->loadIconSet(
              KApplication::reverseLayout() ? "clear_left":"locationbar_erase", K3Icon::Small, 0 ) );
  filterEdit = new KLineEdit(page);
  QLabel *l = new QLabel(filterEdit,i18n("S&earch:"), page);
  connect( clearButton, SIGNAL( clicked() ), filterEdit, SLOT( clear() ) );

  // checkboxes
  noTreeCB = new QCheckBox(i18n("Disable &tree view"), page);
  noTreeCB->setChecked(false);
  subCB = new QCheckBox(i18n("&Subscribed only"), page);
  subCB->setChecked(false);
  newCB = new QCheckBox(i18n("&New only"), page);
  newCB->setChecked(false);


  KSeparator *sep = new KSeparator(Qt::Horizontal, page);

  // init the labels
  QFont fnt = font();
  fnt.setBold(true);
  leftLabel = new QLabel(i18n("Loading..."), page);
  rightLabel = new QLabel(i18n("Current changes:"), page);
  leftLabel->setFont(fnt);
  rightLabel->setFont(fnt);

  // icons
  pmRight = BarIconSet("forward");
  pmLeft = BarIconSet("back");

  arrowBtn1 = new QPushButton(page);
  arrowBtn1->setEnabled(false);
  arrowBtn2 = new QPushButton(page);
  arrowBtn2->setEnabled(false);
  arrowBtn1->setIconSet(pmRight);
  arrowBtn2->setIconSet(pmRight);
  arrowBtn1->setFixedSize(35,30);
  arrowBtn2->setFixedSize(35,30);

  // the main listview
  groupView = new Q3ListView(page);
  groupView->setRootIsDecorated(true);
  groupView->addColumn(i18n("Name"));
  groupView->setAllColumnsShowFocus(true);
  if (descriptionColumn)
    mDescrColumn = groupView->addColumn(i18n("Description"));
  else
    groupView->header()->setStretchEnabled(true, 0);

  // layout
  QGridLayout *topL = new QGridLayout(page,4,1,0, KDialog::spacingHint());
  QHBoxLayout *filterL = new QHBoxLayout(KDialog::spacingHint());
  QVBoxLayout *arrL = new QVBoxLayout(KDialog::spacingHint());
  listL = new QGridLayout(2, 3, KDialog::spacingHint());

  topL->addWidget(comment, 0,0);
  topL->addLayout(filterL, 1,0);
  topL->addWidget(sep,2,0);
  topL->addLayout(listL, 3,0);

  filterL->addWidget(clearButton);
  filterL->addWidget(l);
  filterL->addWidget(filterEdit, 1);
  filterL->addWidget(noTreeCB);
  filterL->addWidget(subCB);
  filterL->addWidget(newCB);

  listL->addWidget(leftLabel, 0,0);
  listL->addWidget(rightLabel, 0,2);
  listL->addWidget(groupView, 1,0);
  listL->addLayout(arrL, 1,1);
  listL->setRowStretch(1,1);
  listL->setColStretch(0,5);
  listL->setColStretch(2,2);

  arrL->addWidget(arrowBtn1, Qt::AlignCenter);
  arrL->addWidget(arrowBtn2, Qt::AlignCenter);

  // listviews
  subView = new Q3ListView(page);
  subView->addColumn(i18n("Subscribe To"));
  subView->header()->setStretchEnabled(true, 0);
  unsubView = new Q3ListView(page);
  unsubView->addColumn(i18n("Unsubscribe From"));
  unsubView->header()->setStretchEnabled(true, 0);

  QVBoxLayout *protL = new QVBoxLayout(3);
  listL->addLayout(protL, 1,2);
  protL->addWidget(subView);
  protL->addWidget(unsubView);

  // disable some widgets as long we're loading
  enableButton(User1, false);
  enableButton(User2, false);
  newCB->setEnabled(false);
  noTreeCB->setEnabled(false);
  subCB->setEnabled(false);

  filterEdit->setFocus();

   // items clicked
  connect(groupView, SIGNAL(clicked(Q3ListViewItem *)),
      this, SLOT(slotChangeButtonState(Q3ListViewItem*)));
  connect(subView, SIGNAL(clicked(Q3ListViewItem *)),
      this, SLOT(slotChangeButtonState(Q3ListViewItem*)));
  connect(unsubView, SIGNAL(clicked(Q3ListViewItem *)),
      this, SLOT(slotChangeButtonState(Q3ListViewItem*)));

  // connect buttons
  connect(arrowBtn1, SIGNAL(clicked()), SLOT(slotButton1()));
  connect(arrowBtn2, SIGNAL(clicked()), SLOT(slotButton2()));
  connect(this, SIGNAL(user1Clicked()), SLOT(slotLoadFolders()));

  // connect checkboxes
  connect(subCB, SIGNAL(clicked()), SLOT(slotCBToggled()));
  connect(newCB, SIGNAL(clicked()), SLOT(slotCBToggled()));
  connect(noTreeCB, SIGNAL(clicked()), SLOT(slotCBToggled()));

  // connect textfield
  connect(filterEdit, SIGNAL(textChanged(const QString&)),
          SLOT(slotFilterTextChanged(const QString&)));

  // update status
  connect(this, SIGNAL(listChanged()), SLOT(slotUpdateStatusLabel()));
}

//-----------------------------------------------------------------------------
KSubscription::~KSubscription()
{
}

//-----------------------------------------------------------------------------
void KSubscription::setStartItem( const KGroupInfo &info )
{
  Q3ListViewItemIterator it(groupView);

  for ( ; it.current(); ++it)
  {
    if (static_cast<GroupItem*>(it.current())->info() == info)
    {
      it.current()->setSelected(true);
      it.current()->setOpen(true);
    }
  }
}

//-----------------------------------------------------------------------------
void KSubscription::removeListItem( Q3ListView *view, const KGroupInfo &gi )
{
  if(!view) return;
  Q3ListViewItemIterator it(view);

  for ( ; it.current(); ++it)
  {
    if (static_cast<GroupItem*>(it.current())->info() == gi)
    {
      delete it.current();
      break;
    }
  }
  if (view == groupView)
    emit listChanged();
}

//-----------------------------------------------------------------------------
Q3ListViewItem* KSubscription::getListItem( Q3ListView *view, const KGroupInfo &gi )
{
  if(!view) return 0;
  Q3ListViewItemIterator it(view);

  for ( ; it.current(); ++it)
  {
    if (static_cast<GroupItem*>(it.current())->info() == gi)
      return (it.current());
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool KSubscription::itemInListView( Q3ListView *view, const KGroupInfo &gi )
{
  if(!view) return false;
  Q3ListViewItemIterator it(view);

  for ( ; it.current(); ++it)
    if (static_cast<GroupItem*>(it.current())->info() == gi)
      return true;

  return false;
}

//------------------------------------------------------------------------------
void KSubscription::setDirectionButton1( Direction dir )
{
  mDirButton1 = dir;
  if (dir == Left)
    arrowBtn1->setIconSet(pmLeft);
  else
    arrowBtn1->setIconSet(pmRight);
}

//------------------------------------------------------------------------------
void KSubscription::setDirectionButton2( Direction dir )
{
  mDirButton2 = dir;
  if (dir == Left)
    arrowBtn2->setIconSet(pmLeft);
  else
    arrowBtn2->setIconSet(pmRight);
}

//------------------------------------------------------------------------------
void KSubscription::changeItemState( GroupItem* item, bool on )
{
  // is this a checkable item
  if (!item->isCheckItem()) return;

  // if we're currently loading the items ignore changes
  if (mLoading) return;
  if (on)
  {
    if (!itemInListView(unsubView, item->info()))
    {
      Q3ListViewItem *p = item->parent();
      while (p)
      {
        // make sure all parents are subscribed
        GroupItem* pi = static_cast<GroupItem*>(p);
        if (pi->isCheckItem() && !pi->isOn())
        {
          pi->setIgnoreStateChange(true);
          pi->setOn(true);
          pi->setIgnoreStateChange(false);
          new GroupItem(subView, pi->info(), this);
        }
        p = p->parent();
      }
      new GroupItem(subView, item->info(), this);
    }
    // eventually remove it from the other listview
    removeListItem(unsubView, item->info());
  }
  else {
    if (!itemInListView(subView, item->info()))
    {
      new GroupItem(unsubView, item->info(), this);
    }
    // eventually remove it from the other listview
    removeListItem(subView, item->info());
  }
  // update the buttons
  slotChangeButtonState(item);
}

//------------------------------------------------------------------------------
void KSubscription::filterChanged( Q3ListViewItem* item, const QString & text )
{
  if ( !item && groupView )
    item = groupView->firstChild();
  if ( !item )
    return;

  do
  {
    if ( item->firstChild() ) // recursive descend
      filterChanged(item->firstChild(), text);

    GroupItem* gr = static_cast<GroupItem*>(item);
    if (subCB->isChecked() || newCB->isChecked() || !text.isEmpty() ||
        noTreeCB->isChecked())
    {
      // set it invisible
      if ( subCB->isChecked() &&
           (!gr->isCheckItem() ||
            (gr->isCheckItem() && !gr->info().subscribed)) )
      {
        // only subscribed
        gr->setVisible(false);
        continue;
      }
      if ( newCB->isChecked() &&
           (!gr->isCheckItem() ||
            (gr->isCheckItem() && !gr->info().newGroup)) )
      {
        // only new
        gr->setVisible(false);
        continue;
      }
      if ( !text.isEmpty() &&
           gr->text(0).find(text, 0, false) == -1)
      {
        // searchfield
        gr->setVisible(false);
        continue;
      }
      if ( noTreeCB->isChecked() &&
           !gr->isCheckItem() )
      {
        // disable treeview
        gr->setVisible(false);
        continue;
      }

      gr->setVisible(true);

    } else {
      gr->setVisible(true);
    }

  } while ((item = item->nextSibling()));

}

//------------------------------------------------------------------------------
int KSubscription::activeItemCount()
{
  Q3ListViewItemIterator it(groupView);

  int count = 0;
  for ( ; it.current(); ++it)
  {
    if (static_cast<GroupItem*>(it.current())->isCheckItem() &&
        it.current()->isVisible() && it.current()->isEnabled())
      count++;
  }

  return count;
}

//------------------------------------------------------------------------------
void KSubscription::restoreOriginalParent()
{
  Q3PtrList<Q3ListViewItem> move;
  Q3ListViewItemIterator it(groupView);
  for ( ; it.current(); ++it)
  {
    Q3ListViewItem* origParent = static_cast<GroupItem*>(it.current())->
      originalParent();
    if (origParent && origParent != it.current()->parent())
    {
      // remember this to avoid messing up the iterator
      move.append(it.current());
    }
  }
  Q3PtrListIterator<Q3ListViewItem> it2( move );
  for ( ; it2.current(); ++it2)
  {
    // restore the original parent
    Q3ListViewItem* origParent = static_cast<GroupItem*>(it2.current())->
      originalParent();
    groupView->takeItem(it2.current());
    origParent->insertItem(it2.current());
  }
}

//-----------------------------------------------------------------------------
void KSubscription::saveOpenStates()
{
  Q3ListViewItemIterator it(groupView);

  for ( ; it.current(); ++it)
  {
    static_cast<GroupItem*>(it.current())->setLastOpenState(
        it.current()->isOpen() );
  }
}

//-----------------------------------------------------------------------------
void KSubscription::restoreOpenStates()
{
  Q3ListViewItemIterator it(groupView);

  for ( ; it.current(); ++it)
  {
    it.current()->setOpen(
        static_cast<GroupItem*>(it.current())->lastOpenState() );
  }
}

//-----------------------------------------------------------------------------
void KSubscription::slotLoadingComplete()
{
  mLoading = false;

  enableButton(User1, true);
  enableButton(User2, true);
  newCB->setEnabled(true);
  noTreeCB->setEnabled(true);
  subCB->setEnabled(true);

  // remember the correct parent
  Q3ListViewItemIterator it(groupView);
  for ( ; it.current(); ++it)
  {
    static_cast<GroupItem*>(it.current())->
      setOriginalParent( it.current()->parent() );
  }

  emit listChanged();
}

//------------------------------------------------------------------------------
void KSubscription::slotChangeButtonState( Q3ListViewItem *item )
{
  if (!item ||
      (item->listView() == groupView &&
       !static_cast<GroupItem*>(item)->isCheckItem()))
  {
    // disable and return
    arrowBtn1->setEnabled(false);
    arrowBtn2->setEnabled(false);
    return;
  }
  // set the direction of the buttons and enable/disable them
  Q3ListView* currentView = item->listView();
  if (currentView == groupView)
  {
    setDirectionButton1(Right);
    setDirectionButton2(Right);
    if (static_cast<GroupItem*>(item)->isOn())
    {
      // already subscribed
      arrowBtn1->setEnabled(false);
      arrowBtn2->setEnabled(true);
    } else {
      // unsubscribed
      arrowBtn1->setEnabled(true);
      arrowBtn2->setEnabled(false);
    }
  } else if (currentView == subView)
  {
    // undo possible
    setDirectionButton1(Left);

    arrowBtn1->setEnabled(true);
    arrowBtn2->setEnabled(false);
  } else if (currentView == unsubView)
  {
    // undo possible
    setDirectionButton2(Left);

    arrowBtn1->setEnabled(false);
    arrowBtn2->setEnabled(true);
  }
}

//------------------------------------------------------------------------------
void KSubscription::slotButton1()
{
  if (mDirButton1 == Right)
  {
    if (groupView->currentItem() &&
        static_cast<GroupItem*>(groupView->currentItem())->isCheckItem())
    {
      // activate
      GroupItem* item = static_cast<GroupItem*>(groupView->currentItem());
      item->setOn(true);
    }
  }
  else {
    if (subView->currentItem())
    {
      GroupItem* item = static_cast<GroupItem*>(subView->currentItem());
      // get the corresponding item from the groupView
      Q3ListViewItem* listitem = getListItem(groupView, item->info());
      if (listitem)
      {
        // deactivate
        GroupItem* chk = static_cast<GroupItem*>(listitem);
        chk->setOn(false);
      }
    }
  }
}

//------------------------------------------------------------------------------
void KSubscription::slotButton2()
{
  if (mDirButton2 == Right)
  {
    if (groupView->currentItem() &&
        static_cast<GroupItem*>(groupView->currentItem())->isCheckItem())
    {
      // deactivate
      GroupItem* item = static_cast<GroupItem*>(groupView->currentItem());
      item->setOn(false);
    }
  }
  else {
    if (unsubView->currentItem())
    {
      GroupItem* item = static_cast<GroupItem*>(unsubView->currentItem());
      // get the corresponding item from the groupView
      Q3ListViewItem* listitem = getListItem(groupView, item->info());
      if (listitem)
      {
        // activate
        GroupItem* chk = static_cast<GroupItem*>(listitem);
        chk->setOn(true);
      }
    }
  }
}

//------------------------------------------------------------------------------
void KSubscription::slotCBToggled()
{
  if (!noTreeCB->isChecked() && !newCB->isChecked() && !subCB->isChecked())
  {
    restoreOriginalParent();
  }
  // set items {in}visible
  filterChanged(groupView->firstChild());
  emit listChanged();
}

//------------------------------------------------------------------------------
void KSubscription::slotFilterTextChanged( const QString & text )
{
  // remember is the items are open
  if (mLastText.isEmpty())
    saveOpenStates();

  if (!mLastText.isEmpty() && text.length() < mLastText.length())
  {
    // reset
    restoreOriginalParent();
    Q3ListViewItemIterator it(groupView);
    for ( ; it.current(); ++it)
    {
      it.current()->setVisible(true);
      it.current()->setEnabled(true);
    }
  }
  // set items {in}visible
  filterChanged(groupView->firstChild(), text);
  // restore the open-states
  if (text.isEmpty())
    restoreOpenStates();

  emit listChanged();
  mLastText = text;
}

//------------------------------------------------------------------------------
void KSubscription::slotUpdateStatusLabel()
{
  QString text;
  if (mLoading)
    text = i18n("Loading... (1 matching)", "Loading... (%n matching)",
                activeItemCount());
  else
    text = i18n("%1: (1 matching)", "%1: (%n matching)", activeItemCount())
           .arg(account()->name());

  leftLabel->setText(text);
}

//------------------------------------------------------------------------------
void KSubscription::slotLoadFolders()
{
  enableButton(User1, false);
  mLoading = true;
  subView->clear();
  unsubView->clear();
  groupView->clear();
}

#include "ksubscription.moc"
