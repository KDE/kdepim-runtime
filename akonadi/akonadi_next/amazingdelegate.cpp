
#include "amazingdelegate.h"

#include <QPainter>

#include <kdebug.h>
#include "entitytreemodel.h"
#include <kabc/addressee.h>

using namespace Akonadi;

AmazingContactItemDelegate::AmazingContactItemDelegate(QObject* parent): QStyledItemDelegate(parent)
{

}

void AmazingContactItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{

  Item item = index.data(EntityTreeModel::ItemRole).value<Item>();

  if (!item.isValid())
      QStyledItemDelegate::paint(painter, option, index);

  KABC::Addressee addressee = item.payload<KABC::Addressee>();

  painter->save();

  if (option.state & QStyle::State_Selected)
    painter->fillRect(option.rect, option.palette.highlight());

  int PaintingScaleFactor = 2;
  int yOffset = (option.rect.height() - PaintingScaleFactor) / 2;
  painter->translate(option.rect.x(), option.rect.y() + yOffset);

  painter->drawText(50, 0, addressee.formattedName());
  painter->drawText(50, 15, addressee.preferredEmail());


  painter->restore();

}


QSize AmazingContactItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  Item item = index.data(EntityTreeModel::ItemRole).value<Item>();

  if (!item.isValid())
    return QStyledItemDelegate::sizeHint(option, index);

  QSize s = QStyledItemDelegate::sizeHint(option, index);
  s.setHeight( s.height() * 2.5);
  return s;

}

