/*
    Copyright (C) 2004, David Faure <faure@kde.org>
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

#include "kpartsdesignerplugin.h"

#include <kparts/componentfactory.h>
#include <kparts/part.h>
#include <kmimetype.h>
#include <qlayout.h>
#include <kapplication.h>
#include <kdepimmacros.h>

KPartsGenericPart::KPartsGenericPart( QWidget* parentWidget, const char* name )
    : QWidget( parentWidget, name ), m_part( 0 )
{
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setAutoAdd( true );
}

void KPartsGenericPart::load()
{
    if ( m_mimetype.isEmpty() || m_url.isEmpty() )
        return; // not enough info yet
    // Here it crashes in KSycoca::openDatabase when trying to load the stuff from designer itself
    // Not sure why - but it's not really needed anyway.
    if ( !kapp )
        return;
    QString mimetype = m_mimetype;
    if ( mimetype == "auto" )
        mimetype == KMimeType::findByURL( m_url )->name();
    if ( m_part ) {
        delete m_part;
    }
    // "this" is both the parent widget and the parent object
    m_part = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>( mimetype, QString::null, this, 0, this, 0 );
    if ( m_part ) {
        m_part->openURL( m_url );
        m_part->widget()->show();
    }
}

////

static const char* mykey = "KPartsGenericPart";

QStringList KPartsWidgetPlugin::keys() const {
    return QStringList() << mykey;
}

QWidget * KPartsWidgetPlugin::create( const QString & key, QWidget * parent, const char * name ) {
    if ( key == mykey )
        return new KPartsGenericPart( parent, name );
    return 0;
}

QString KPartsWidgetPlugin::group( const QString & key ) const {
    if ( key == mykey )
        return "Display (KDE)";
    return QString::null;
}

#if 0
QIconSet KPartsWidgetPlugin::iconSet( const QString & key ) const {
  return QIconSet();
}
#endif

QString KPartsWidgetPlugin::includeFile( const QString & key ) const {
    if ( key == mykey )
        return "partplugin.h";
    return QString::null;
}

QString KPartsWidgetPlugin::toolTip( const QString & key ) const {
    if ( key == mykey )
        return "Generic KParts viewer";
    return QString::null;
}

QString KPartsWidgetPlugin::whatsThis( const QString & key ) const {
    if ( key == mykey )
        return "A widget to embed any KParts viewer, given a url and optionally a mimetype";
    return QString::null;
}

bool KPartsWidgetPlugin::isContainer( const QString & /*key*/ ) const {
    return false;
}

/// Duplicated from kdelibs/kdecore/kdemacros.h.in for those with kdelibs < 3.4
#ifndef KDE_Q_EXPORT_PLUGIN
#define KDE_Q_EXPORT_PLUGIN(PLUGIN) \
  Q_EXTERN_C KDE_EXPORT const char* qt_ucm_query_verification_data(); \
  Q_EXTERN_C KDE_EXPORT QUnknownInterface* ucm_instantiate(); \
  Q_EXPORT_PLUGIN(PLUGIN)
#endif

KDE_Q_EXPORT_PLUGIN( KPartsWidgetPlugin )

#include "kpartsdesignerplugin.moc"

