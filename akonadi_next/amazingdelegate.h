

#ifndef AMAZING_DELEGATE_H
#define AMAZING_DELEGATE_H

#include <QStyledItemDelegate>

#include "akonadi_next_export.h"

class AKONADI_NEXT_EXPORT AmazingContactItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  AmazingContactItemDelegate(QObject* parent = 0);

  void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

  virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

};

#endif
