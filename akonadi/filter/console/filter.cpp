/******************************************************************************
 *
 *  File : filter.cpp
 *  Created on Sat 13 Jun 2009 06:08:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "filter.h"

#include <akonadi/filter/program.h>

#include <akonadi/collection.h>

#include <QtGui/QTextDocument>
#include <QtGui/QPalette>

#include <QtCore/QUrl>

#include <KIcon>
#include <KLocale>

Filter::Filter()
{
  mProgram = 0;
  mEditorFactory = 0;
  mComponentFactory = 0;
  mDescriptionDocument = 0;
}

Filter::~Filter()
{
  if( mDescriptionDocument )
    delete mDescriptionDocument;
  if( mProgram )
    delete mProgram;
  qDeleteAll( mCollections );
}

QTextDocument * Filter::descriptionDocument( const QString &textColor )
{
  if( mDescriptionDocument )
  {
    if( mDescriptionDocumentTextColor == textColor )
      return mDescriptionDocument;

    delete mDescriptionDocument;
  }

  mDescriptionDocumentTextColor = textColor;
  mDescriptionDocument = new QTextDocument( 0 );

  QString icon = mProgram ? mProgram->property( QString::fromAscii( "icon" ) ).toString() : QString();
  if( icon.isEmpty() )
    icon = QString::fromAscii( "akonadi" );

  mDescriptionDocument->addResource(
      QTextDocument::ImageResource,
      QUrl( QLatin1String( "filter_icon" ) ),
      //KIcon( "view-filter" ).pixmap( QSize( 64, 64 ) )
      KIcon( icon ).pixmap( QSize( 64, 64 ) )
    );
 

  QString name = mProgram ? mProgram->name() : QString::fromAscii( "???" );
  if( name.isEmpty() )
    name = mId;

  QString coll;

  foreach( Akonadi::Collection * c, mCollections )
  {
    if( !coll.isEmpty() )
      coll += QString::fromAscii( ", " );
    coll += c->name();
  }

  if( coll.isEmpty() )
    coll = i18n( "none" );
  else if( coll.length() > 80 )
    coll = coll.left( 80 ) + QString::fromAscii( "..." );

  QString content = QString::fromLatin1(
      "<html style=\"color:%1\">"
        "<body>"
          "<table>"
            "<tr>"
              "<td rowspan=\"4\"><img src=\"filter_icon\">&nbsp;&nbsp;</td>"
              "<td><b>%2</b></td>"
            "</tr>"
            "<tr>"
              "<td>%3: %4</td>"
            "</tr>"
            "<tr>"
              "<td><font size=\"-1\">%5: %6</font></td>"
            "</tr>"
            "<tr>"
              "<td><font size=\"-1\">%7: %8</font></td>"
            "</tr>"
          "</table>" 
        "</body>" 
      "</html>"
    )
      .arg( textColor )
      .arg( name )
      .arg( i18n( "Id" ) )
      .arg( mId )
      .arg( i18n( "Mime-Type" ) )
      .arg( mMimeType )
      .arg( i18n( "Collections" ) )
      .arg( coll );

  mDescriptionDocument->setHtml( content );

  return mDescriptionDocument;

}

void Filter::invalidateDescriptionDocument()
{
  if( mDescriptionDocument )
    delete mDescriptionDocument;
  mDescriptionDocument = 0;
}

void Filter::setId( const QString &id )
{
  mId = id;
  invalidateDescriptionDocument();
}

void Filter::setMimeType( const QString &mimeType )
{
  mMimeType = mimeType;
  invalidateDescriptionDocument();
}

void Filter::setProgram( Akonadi::Filter::Program * prog )
{
  Q_ASSERT( prog );
  if( mProgram )
    delete mProgram;
  mProgram = prog;
  invalidateDescriptionDocument();
}

Akonadi::Collection * Filter::findCollection( Akonadi::Collection::Id id )
{
  foreach( Akonadi::Collection * c, mCollections )
  {
    if( c->id() == id )
      return c;
  }

  return 0;
}

bool Filter::hasCollection( Akonadi::Collection::Id id )
{
  foreach( Akonadi::Collection * c, mCollections )
  {
    if( c->id() == id )
      return true;
  }

  return false;
}

QList< Akonadi::Collection::Id > Filter::collectionsAsIdList()
{
  QList< Akonadi::Collection::Id > ret;
  foreach( Akonadi::Collection * c, mCollections )
    ret.append( c->id() );

  return ret;
}


void Filter::addCollection( Akonadi::Collection * collection )
{
  Q_ASSERT( !hasCollection( collection->id() ) );
  mCollections.append( collection );
  invalidateDescriptionDocument();
}

void Filter::removeCollection( Akonadi::Collection::Id id )
{
  Akonadi::Collection * c = findCollection( id );
  Q_ASSERT( c );
  mCollections.removeOne( c );
  delete c;
  invalidateDescriptionDocument();
}

void Filter::removeAllCollections()
{
  qDeleteAll(mCollections);
  mCollections.clear();
  invalidateDescriptionDocument();
}

