/**
 * krichtext_plugin.cpp
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 */
#include "krichtext_plugin.h"
#include "core.h"

#include <kparts/componentfactory.h>
#include <kgenericfactory.h>
#include <kapplication.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qwidget.h>

typedef KGenericFactory<KRichTextPlugin, Komposer::Core> KRichTextPluginFactory;
K_EXPORT_COMPONENT_FACTORY( libkomposer_krichtextplugin,
                            KRichTextPluginFactory( "komposer_krichtextplugin" ) )

KRichTextPlugin::KRichTextPlugin( Komposer::Core* core, const char* name, const QStringList& )
  : Editor( core, core, name ), m_part( 0 )
{
  setInstance( KRichTextPluginFactory::instance() );
}

KRichTextPlugin::~KRichTextPlugin()
{
}


KParts::Part*
KRichTextPlugin::part()
{
  if ( !m_part ) {
    kdDebug() << "KRichText_Plugin: No part!!!" << endl;
    m_part = KParts::ComponentFactory
            ::createPartInstanceFromLibrary<KParts::ReadWritePart>( "libkrichtexteditpart",
                                                                    core(), "krichtext", // parentwidget,name
                                                                    this, 0 ); // parent,name
    if ( !m_part ) {
      kdWarning()<<"Big problem"<<endl;
    }

    return m_part;
  } else
    return m_part;
}

QString
KRichTextPlugin::text() const
{
  return QString();
}

void
KRichTextPlugin::setText( const QString& txt )
{
}

void
KRichTextPlugin::changeSignature( const QString& txt )
{

}

#include "krichtext_plugin.moc"
