/*
    Copyright (C) 2005, David Faure <faure@kde.org>
    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2, as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/


#ifndef DESIGNER_PARTPLUGIN_H
#define DESIGNER_PARTPLUGIN_H

#include <qwidgetplugin.h>
#include <qwidget.h>
namespace KParts { class ReadOnlyPart; }

/**
 * Generic part loader, able to view any kind of file for which
 * a KParts::ReadOnlyPart is available
 */
class KPartsGenericPart : public QWidget {
    Q_OBJECT
    Q_PROPERTY( QString url READ url WRITE setURL )
    Q_PROPERTY( QString mimetype READ mimetype WRITE setMimetype )
public:
    KPartsGenericPart( QWidget* parentWidget, const char* name );

    QString url() const { return m_url; }
    void setURL( const QString& url ) { m_url = url; load(); }

    // The mimetype, or "auto" if unknown
    QString mimetype() const { return m_mimetype; }
    void setMimetype( const QString& mimetype ) { m_mimetype = mimetype; load(); }

private:
    void load();

private:
    QString m_mimetype;
    QString m_url;
    KParts::ReadOnlyPart* m_part;
};

/**
 * Qt designer plugin for embedding a KParts using KPartsGenericPart
 */
class KPartsWidgetPlugin : public QWidgetPlugin {
public:
  QStringList keys() const;
  QWidget * create( const QString & key, QWidget * parent, const char * name );
  QString group( const QString & key ) const;
  //QIconSet iconSet( const QString & key ) const;
  QString includeFile( const QString & key ) const;
  QString toolTip( const QString & key ) const;
  QString whatsThis( const QString & key ) const;
  bool isContainer( const QString & key ) const;
};

#endif
