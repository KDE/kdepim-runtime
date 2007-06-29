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

#include <plasma/svg.h>

PlasmoBiff::PlasmoBiff(QObject *parent, const QStringList &args)
  : Plasma::Applet(parent, args),
    m_dialog(0)
{
  setFlags(QGraphicsItem::ItemIsMovable);

  KConfigGroup cg = appletConfig();
  m_xPixelSize = cg.readEntry("xsize", 413);
  m_yPixelSize = cg.readEntry("ysize", 307);

  m_theme = new Plasma::Svg("widgets/akonadi", this);
  m_theme->setContentType(Plasma::Svg::SingleImage);
  m_theme->resize(m_xPixelSize,m_yPixelSize);

  // DataEngine
  Plasma::DataEngine* akonadiEngine = dataEngine("akonadi");
  akonadiEngine->connectSource(m_email, this);

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

void PlasmoBiff::configureDialog()
{
  if (m_dialog == 0) {
    m_dialog = new KDialog;
    m_dialog->setCaption( "Configure PlasmoBiff" );

    ui.setupUi(m_dialog->mainWidget());
    m_dialog->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
    connect( m_dialog, SIGNAL(applyClicked()), this, SLOT(configAccepted()) );
    connect( m_dialog, SIGNAL(okClicked()), this, SLOT(configAccepted()) );

  }
  m_dialog->show();
}

void PlasmoBiff::paintInterface(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{

  painter->setRenderHint(QPainter::SmoothPixmapTransform);

  m_theme->paint(painter, boundingRect(), "email_frame");

}

QRectF PlasmoBiff::boundingRect() const
{
  return m_bounds;
}

#include "plasmobiff.moc"
