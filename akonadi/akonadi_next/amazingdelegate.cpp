/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "amazingdelegate.h"

#include <QPainter>

#include <kdebug.h>
#include <akonadi/entitytreemodel.h>
#include <kabc/addressee.h>
#include <klocalizedstring.h>
#include <akonadi/entitydisplayattribute.h>

using namespace Akonadi;

AmazingContactItemDelegate::AmazingContactItemDelegate(QObject* parent): QStyledItemDelegate(parent)
{

}

void AmazingContactItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{

  Item item = index.data(EntityTreeModel::ItemRole).value<Item>();

  if (!item.isValid())
      QStyledItemDelegate::paint(painter, option, index);

  if (!item.hasPayload<KABC::Addressee>())
  {
    kWarning() << "Not a KABC::Addressee" << item.id() << item.remoteId();
    return;
  }

  KABC::Addressee addressee = item.payload<KABC::Addressee>();

  painter->save();

  if (option.state & QStyle::State_Selected)
    painter->fillRect(option.rect, option.palette.highlight());

  int PaintingScaleFactor = 2;
  int yOffset = (option.rect.height() - PaintingScaleFactor) / 2;
  painter->translate(option.rect.x(), option.rect.y() + yOffset);

  Collection parentCollection = index.data(EntityTreeModel::ParentCollection).value<Collection>();

  QString name = addressee.givenName() + " " + addressee.familyName();

  KABC::Picture pic =addressee.photo();

  if (!pic.isEmpty())
  {
    QImage image = pic.data();
    painter->drawImage(QRect(0, 0, 40, 40), image);
  }
 
  QString parentName;
  if (parentCollection.hasAttribute<EntityDisplayAttribute>() && !parentCollection.attribute<EntityDisplayAttribute>()->displayName().isEmpty())
  {
    parentName = parentCollection.attribute<EntityDisplayAttribute>()->displayName();
  } else
  {
    parentName = parentCollection.name();
  }

  QString email = addressee.preferredEmail();
  if (email.isEmpty())
    email = i18nc("An email address is not known for a contact. This is the default text provided", "<no email>");

  painter->drawText(50, 0, name);
  painter->drawText(50, 15, email);
  painter->drawText(50, 30, parentName);


  painter->restore();

}


QSize AmazingContactItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  Item item = index.data(EntityTreeModel::ItemRole).value<Item>();

  if (!item.isValid())
    return QStyledItemDelegate::sizeHint(option, index);

  QSize s = QStyledItemDelegate::sizeHint(option, index);
  s.setHeight( s.height() * 4.5);
  return s;

}

