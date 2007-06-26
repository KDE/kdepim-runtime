/***************************************************************************
 *   Copyright (C) 2007 by Thomas Moenicke                                 *
 *   thomas.moenicke@kdemail.net                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "plasmobiff.h"

#include <QApplication>
#include <QImage>
#include <QPaintDevice>
#include <QLabel>
#include <QPixmap>
#include <QPaintEvent>
#include <QPainter>
#include <QX11Info>
#include <QWidget>
#include <QGraphicsItem>
#include <QColor>

PlasmoBiff::PlasmoBiff(QObject *parent, const QStringList &args)
  : Plasma::Applet(parent, args)
{
  setFlags(QGraphicsItem::ItemIsMovable);

  // DataEngine

  constraintsUpdated();

}

PlasmoBiff::~ PlasmoBiff()
{
}

void PlasmoBiff::constraintsUpdated()
{
  prepareGeometryChange();

  if (formFactor() == Plasma::Planar ||
      formFactor() == Plasma::MediaCenter) {
    QSize s = m_theme->size();
    m_bounds = QRect(0, 0, s.width(), s.height());
  } else {
    QFontMetrics fm(QApplication::font());
    m_bounds = QRectF(0 ,0 ,fm.width("email@something.org"), fm.height() * 1.5);
  }

}

void PlasmoBiff::paintInterface(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
}

QRectF PlasmoBiff::boundingRect() const
{
  return m_bounds;
}

#include "plasmobiff.moc"
