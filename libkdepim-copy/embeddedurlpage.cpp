/*
    This file is part of libkdepim.

    Copyright (c) 2005 Reinhold Kainhofer <reinhold@kainhofer.com>
    Part of loadContents() copied from the kpartsdesignerplugin:
    Copyright (C) 2005, David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "embeddedurlpage.h"
#include <kparts/componentfactory.h>
#include <kparts/browserextension.h>
#include <kparts/part.h>
#include <kmimetype.h>
#include <klocale.h>
#include <qlayout.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QVBoxLayout>

using namespace KPIM;

EmbeddedURLPage::EmbeddedURLPage( const QString &url, const QString &mimetype,
                                  QWidget *parent )
  : QWidget( parent ), mUri(url), mMimeType( mimetype ), mPart( 0 )
{
  initGUI( url, mimetype );
}

void EmbeddedURLPage::initGUI( const QString &url, const QString &/*mimetype*/ )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setAutoAdd( true );
  new QLabel( i18n("Showing URL %1", url ), this );
}

void EmbeddedURLPage::loadContents()
{
  if ( !mPart ) {
    if ( mMimeType.isEmpty() || mUri.isEmpty() )
        return;
    QString mimetype = mMimeType;
    if ( mimetype == "auto" )
        mimetype == KMimeType::findByURL( mUri )->name();
    // "this" is both the parent widget and the parent object
    mPart = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>( mimetype, QString(), this, 0, this, 0 );
    if ( mPart ) {
        mPart->openURL( mUri );
        mPart->widget()->show();
    }
//void KParts::BrowserExtension::openURLRequestDelayed( const KUrl &url, const KParts::URLArgs &args = KParts::URLArgs() )
    KParts::BrowserExtension* be = KParts::BrowserExtension::childObject( mPart );
    connect( be, SIGNAL( openURLRequestDelayed( const KUrl &, const KParts::URLArgs & ) ),
//              mPart, SLOT( openURL( const KUrl & ) ) );
             this, SIGNAL( openURL( const KUrl & ) ) );
  }
}

#include "embeddedurlpage.moc"
