/*
    This file is part of libkdepim.

    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <kabc/address.h>
#include <kabc/addressee.h>
#include <kabc/phonenumber.h>
#include <kapplication.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstringhandler.h>

#include "addresseeview.h"

using namespace KPIM;

AddresseeView::AddresseeView( QWidget *parent, const char *name )
  : KTextBrowser( parent, name )
{
  setWrapPolicy( QTextEdit::AtWordBoundary );
  setLinkUnderline( false );
  setVScrollBarMode( QScrollView::AlwaysOff );
  setHScrollBarMode( QScrollView::AlwaysOff );

  QStyleSheet *sheet = styleSheet();
  QStyleSheetItem *link = sheet->item( "a" );
  link->setColor( KGlobalSettings::linkColor() );

  connect( this, SIGNAL( mailClick( const QString&, const QString& ) ),
           this, SLOT( mailClicked( const QString&, const QString& ) ) );
  connect( this, SIGNAL( urlClick( const QString& ) ),
           this, SLOT( urlClicked( const QString& ) ) );

  setNotifyClick( true );
}

void AddresseeView::setAddressee( const KABC::Addressee& addr )
{
  mAddressee = addr;

  // clear view
  setText( QString::null );

  if ( mAddressee.isEmpty() )
    return;

  QString name = ( mAddressee.formattedName().isEmpty() ?
                   mAddressee.assembledName() : mAddressee.formattedName() );

  QString dynamicPart;

  KABC::PhoneNumber::List phones = mAddressee.phoneNumbers();
  KABC::PhoneNumber::List::ConstIterator phoneIt;
  for ( phoneIt = phones.begin(); phoneIt != phones.end(); ++phoneIt ) {
    QString number = (*phoneIt).number();
    QString strippedNumber;
    for ( uint i = 0; i < number.length(); ++i ) {
      if ( number[ i ].isDigit() )
        strippedNumber.append( number[ i ] );
    }

    dynamicPart += QString(
      "<tr><td align=\"right\"><b>%1</b></td>"
      "<td align=\"left\"><a href=\"phone:%2\">%3</a></td></tr>" )
      .arg( KABC::PhoneNumber::typeLabel( (*phoneIt).type() ) )
      .arg( strippedNumber )
      .arg( number );
  }

  QStringList emails = mAddressee.emails();
  QStringList::ConstIterator emailIt;
  QString type = i18n( "Email" );
  for ( emailIt = emails.begin(); emailIt != emails.end(); ++emailIt ) {
    dynamicPart += QString(
      "<tr><td align=\"right\"><b>%1</b></td>"
      "<td align=\"left\"><a href=\"mailto:%2\">%3</a></td></tr>" )
      .arg( type )
      .arg( *emailIt )
      .arg( *emailIt );
    type = i18n( "Other" );
  }

  if ( !mAddressee.url().url().isEmpty() ) {
    dynamicPart += QString(
      "<tr><td align=\"right\"><b>%1</b></td>"
      "<td align=\"left\">%2</td></tr>" )
      .arg( i18n( "Homepage" ) )
      .arg( KStringHandler::tagURLs( mAddressee.url().url() ) );
  }

  KABC::Address::List addresses = mAddressee.addresses();
  KABC::Address::List::ConstIterator addrIt;
  for ( addrIt = addresses.begin(); addrIt != addresses.end(); ++addrIt ) {
    if ( (*addrIt).label().isEmpty() ) {
      QString formattedAddress = (*addrIt).formattedAddress().stripWhiteSpace();
      formattedAddress = formattedAddress.replace( '\n', "<br>" );

      dynamicPart += QString(
        "<tr><td align=\"right\"><b>%1</b></td>"
        "<td align=\"left\">%2</td></tr>" )
        .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
        .arg( formattedAddress );
    } else {
      dynamicPart += QString(
        "<tr><td align=\"right\"><b>%1</b></td>"
        "<td align=\"left\">%2</td></tr>" )
        .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
        .arg( (*addrIt).label().replace( '\n', "<br>" ) );
    }
  }

  QString notes;
  if ( !mAddressee.note().isEmpty() ) {
    notes = QString(
      "<tr><td colspan=\"2\"><hr noshade=\"1\"></td></tr>"
      "<tr>"
      "<td align=\"right\" valign=\"top\"><b>%1:</b></td>"  // note label
      "<td align=\"left\">%2</td>"  // note
      "</tr>" ).arg( i18n( "Notes" ) ).arg( mAddressee.note().replace( '\n', "<br>" ) );
  }

  QString strAddr = QString::fromLatin1(
  "<html>"
  "<body text=\"%1\" bgcolor=\"%2\">" // text and background color
  "<table>"
  "<tr>"
  "<td rowspan=\"3\" align=\"right\" valign=\"top\">"
  "<img src=\"myimage\" width=\"50\" height=\"70\">"
  "</td>"
  "<td align=\"left\"><font size=\"+2\"><b>%3</b></font></td>"  // name
  "</tr>"
  "<tr>"
  "<td align=\"left\">%4</td>"  // role
  "</tr>"
  "<tr>"
  "<td align=\"left\">%5</td>"  // organization
  "</tr>"
  "<tr><td colspan=\"2\">&nbsp;</td></tr>"
  "%6"  // dynamic part
  "%7"  // notes
  "</table>"
  "</body>"
  "</html>").arg( KGlobalSettings::textColor().name() )
  .arg( KGlobalSettings::baseColor().name() )
  .arg( name )
  .arg( mAddressee.role() )
  .arg( mAddressee.organization() )
  .arg( dynamicPart )
  .arg( notes );

  KABC::Picture picture = mAddressee.photo();
  if ( picture.isIntern() && !picture.data().isNull() )
    QMimeSourceFactory::defaultFactory()->setImage( "myimage", picture.data() );
  else
    QMimeSourceFactory::defaultFactory()->setPixmap( "myimage",
      KGlobal::iconLoader()->loadIcon( "penguin", KIcon::Desktop, 128 ) );

  // at last display it...
  setText( strAddr );
}

KABC::Addressee AddresseeView::addressee() const
{
  return mAddressee;
}

void AddresseeView::mailClicked( const QString&, const QString &email )
{
  kapp->invokeMailer( email, QString::null );
}

void AddresseeView::urlClicked( const QString &url )
{
  if ( url.startsWith( "phone:" ) )
    emit phoneNumberClicked( url.mid( 8 ) );
  else
    kapp->invokeBrowser( url );
}

#include "addresseeview.moc"
