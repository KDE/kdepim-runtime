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
    m_dialog(0),
    m_fmFrom(QApplication::font()),
    m_fmSubject(QApplication::font())
{
  setFlags(QGraphicsItem::ItemIsMovable);

  KConfigGroup cg = config();
  m_xPixelSize = cg.readEntry("xsize", 413);
  m_yPixelSize = cg.readEntry("ysize", 307);

  m_theme = new Plasma::Svg("widgets/akonadi", this);
  m_theme->setContentType(Plasma::Svg::SingleImage);
  m_theme->resize(m_xPixelSize,m_yPixelSize);

  // DataEngine
  engine = dataEngine("akonadi");
  engine->connectAllSources(this);
  connect( engine, SIGNAL(newSource(QString)), SLOT(newSource(QString)) );

  constraintsUpdated();

  QFontMetrics fmFrom(QApplication::font());
  QFontMetrics fmSubject(QApplication::font());

  m_fontFrom.setPointSize(9);
  m_fontFrom.setFamily("Sans Serif");

  m_fontSubject.setBold(true);
  m_fontSubject.setPointSize(9);
  m_fontSubject.setFamily("Sans Serif");

  m_subjectList[0] = QString("hello aKademy");

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
  // draw the main background stuff
  m_theme->paint(painter, boundingRect(), "email_frame");

  QRectF bRect = boundingRect();
  bRect.setX(bRect.x()+93);

  // draw the 4 channels
  bRect.setY(bRect.y()+102);
  drawEmail(0, bRect, painter);

  bRect.setY(bRect.y()-88);
  drawEmail(1, bRect, painter);

  bRect.setY(bRect.y()-88);
  drawEmail(2, bRect, painter);

  bRect.setY(bRect.y()-88);
  drawEmail(3, bRect, painter);

}

QRectF PlasmoBiff::boundingRect() const
{
  return m_bounds;
}

void PlasmoBiff::drawEmail(int index, const QRectF& rect, QPainter* painter)
{

  QPen _pen = painter->pen();
  QFont _font = painter->font();

  painter->setPen(Qt::white);

  QString from = m_fromList[index];
  if(from.size() > 33) // cut if too long
    {
      from.resize(30);
      from.append("...");
    }
  //  qDebug() << m_fontFrom.family() << m_fontFrom.pixelSize() << m_fontFrom.pointSize();
  painter->setFont(m_fontFrom);
  painter->drawText((int)(rect.width()/2 - m_fmFrom.width(from) / 2),
		    (int)((rect.height()/2) - m_fmFrom.xHeight()*3),
		    from);

  QString subject = m_subjectList[index];
  if(subject.size() > 33) // cut
    {
      subject.resize(30);
      subject.append("...");
    }

  painter->setFont(m_fontSubject);
  painter->drawText((int)(rect.width()/2 - m_fmSubject.width(subject) / 2),
		    (int)((rect.height()/2) - m_fmSubject.xHeight()*3 + 15),
		    subject);

  // restore
  painter->setFont(_font);
  painter->setPen(_pen);

}

void PlasmoBiff::updated(const QString &source, const Plasma::DataEngine::Data &data)
{
  m_fromList[3] = m_fromList[2];
  m_subjectList[3] = m_subjectList[2];

  m_fromList[2] = m_fromList[1];
  m_subjectList[2] = m_subjectList[1];

  m_fromList[1] = m_fromList[0];
  m_subjectList[1] = m_subjectList[0];

  m_fromList[0] = data["From"].toString();
  m_subjectList[0] = data["Subject"].toString();

  update();
}

void PlasmoBiff::newSource(const QString & source)
{
  engine->connectSource( source, this );
}

#include "plasmobiff.moc"
