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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kpartsdesignerplugin.h"

#include <kparts/componentfactory.h>
#include <kparts/part.h>
#include <kmimetype.h>
#include <QLayout>
//Added by qt3to4:
#include <QVBoxLayout>
#include <qplugin.h>
#include <kapplication.h>
#include <kdepimmacros.h>

KPartsGenericPart::KPartsGenericPart( QWidget* parentWidget, const char* name )
    : QWidget( parentWidget ), m_part( 0 )
{
    setObjectName( name );
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
    m_part = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>( mimetype, QString(), this, this );
    if ( m_part ) {
        m_part->openURL( m_url );
        m_part->widget()->show();
    }
}

////

QWidget * KPartsWidgetPlugin::createWidget( QWidget * parent ) {
    return new KPartsGenericPart( parent );
}

QString KPartsWidgetPlugin::group() const {
    return "Display (KDE)";
}

QIcon KPartsWidgetPlugin::icon() const {
  return QIcon();
}

QString KPartsWidgetPlugin::includeFile() const {
    return "partplugin.h";
}

QString KPartsWidgetPlugin::toolTip() const {
    return "Generic KParts viewer";
}

QString KPartsWidgetPlugin::whatsThis() const {
    return "A widget to embed any KParts viewer, given a url and optionally a mimetype";
}

bool KPartsWidgetPlugin::isContainer() const {
    return false;
}

QString KPartsWidgetPlugin::name() const {
    return "KParts";
}

Q_EXPORT_PLUGIN( KPartsWidgetPlugin )

#include "kpartsdesignerplugin.moc"

