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

#include <qpopupmenu.h>

#include <kabc/address.h>
#include <kabc/addressee.h>
#include <kabc/phonenumber.h>
#include <kactionclasses.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kstringhandler.h>

#include "addresseeview.h"

using namespace KPIM;

AddresseeView::AddresseeView( QWidget *parent, const char *name,
                              KConfig *config )
  : KTextBrowser( parent, name ), mDefaultConfig( false ), mImageJob( 0 )
{
  setWrapPolicy( QTextEdit::AtWordBoundary );
  setLinkUnderline( false );
  setVScrollBarMode( QScrollView::AlwaysOff );
  setHScrollBarMode( QScrollView::AlwaysOff );

  QStyleSheet *sheet = styleSheet();
  QStyleSheetItem *link = sheet->item( "a" );
  link->setColor( KGlobalSettings::linkColor() );

  connect( this, SIGNAL( mailClick( const QString&, const QString& ) ),
           this, SLOT( slotMailClicked( const QString&, const QString& ) ) );
  connect( this, SIGNAL( urlClick( const QString& ) ),
           this, SLOT( slotUrlClicked( const QString& ) ) );
  connect( this, SIGNAL( highlighted( const QString& ) ),
           this, SLOT( slotHighlighted( const QString& ) ) );

  setNotifyClick( true );

  mActionShowBirthday = new KToggleAction( i18n( "Show Birthday" ) );
  mActionShowAddresses = new KToggleAction( i18n( "Show Postal Addresses" ) );
  mActionShowEmails = new KToggleAction( i18n( "Show Email Addresses" ) );
  mActionShowPhones = new KToggleAction( i18n( "Show Telephone Numbers" ) );
  mActionShowURLs = new KToggleAction( i18n( "Show Web Pages (URLs)" ) );

  connect( mActionShowBirthday, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowAddresses, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowEmails, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowPhones, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowURLs, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );

  if ( !config ) {
    mConfig = new KConfig( "kaddressbookrc" );
    mDefaultConfig = true;
  } else
    mConfig = config;

  load();
}

AddresseeView::~AddresseeView()
{
  if ( mDefaultConfig )
    delete mConfig;
  mConfig = 0;

  delete mActionShowBirthday;
  delete mActionShowAddresses;
  delete mActionShowEmails;
  delete mActionShowPhones;
  delete mActionShowURLs;
}

void AddresseeView::setAddressee( const KABC::Addressee& addr )
{
  mAddressee = addr;

  if ( mImageJob ) {
    mImageJob->kill();
    mImageJob = 0;
  }

  mImageData.truncate( 0 );

  updateView();
}

void AddresseeView::updateView()
{
  // clear view
  setText( QString::null );

  if ( mAddressee.isEmpty() ) {
    QMimeSourceFactory::defaultFactory()->setImage( "myimage", QByteArray() );
    return;
  }

  if ( mImageJob ) {
    mImageJob->kill();
    mImageJob = 0;

    mImageData.truncate( 0 );
  }

  QString name = ( mAddressee.formattedName().isEmpty() ?
                   mAddressee.assembledName() : mAddressee.formattedName() );

  QString dynamicPart;

  QString rowFmtStr = QString::fromLatin1(
			"<tr><td align=\"right\" width=\"30%\"><b>%1</b></td>"
			"<td align=\"left\" width=\"70%\">%2</td></tr>\n"
			);

  if ( mActionShowBirthday->isChecked() ) {
    QDate date = mAddressee.birthday().date();

    if ( date.isValid() )
      dynamicPart += rowFmtStr
        .arg( KABC::Addressee::birthdayLabel() )
        .arg( KGlobal::locale()->formatDate( date, true ) );
  }

  if ( mActionShowPhones->isChecked() ) {
    KABC::PhoneNumber::List phones = mAddressee.phoneNumbers();
    KABC::PhoneNumber::List::ConstIterator phoneIt;
    for ( phoneIt = phones.begin(); phoneIt != phones.end(); ++phoneIt ) {
      QString number = (*phoneIt).number();

      QString url;
      if ( (*phoneIt).type() & KABC::PhoneNumber::Fax )
        url = QString::fromLatin1("fax:") + number;
      else
        url = QString::fromLatin1("phone:") + number;

      dynamicPart += rowFmtStr
        .arg( KABC::PhoneNumber::typeLabel( (*phoneIt).type() ).replace( " ", "&nbsp;" ) )
        .arg( QString::fromLatin1("<a href=\"%1\">%2</a>").arg(url).arg(number) );
    }
  }

  if ( mActionShowEmails->isChecked() ) {
    QStringList emails = mAddressee.emails();
    QStringList::ConstIterator emailIt;
    QString type = i18n( "Email" );
    for ( emailIt = emails.begin(); emailIt != emails.end(); ++emailIt ) {
      dynamicPart += rowFmtStr
        .arg( type )
        .arg( QString::fromLatin1("<a href=\"mailto:%1\">%2</a>").arg(*emailIt).arg(*emailIt) );
      type = i18n( "Other" );
    }
  }

  if ( mActionShowURLs->isChecked() ) {
    if ( !mAddressee.url().url().isEmpty() ) {
      dynamicPart += rowFmtStr
        .arg( i18n( "Homepage" ) )
        .arg( KStringHandler::tagURLs( mAddressee.url().url() ) );
    }
  }

  if ( mActionShowAddresses->isChecked() ) {
    KABC::Address::List addresses = mAddressee.addresses();
    KABC::Address::List::ConstIterator addrIt;
    for ( addrIt = addresses.begin(); addrIt != addresses.end(); ++addrIt ) {
      if ( (*addrIt).label().isEmpty() ) {
        QString formattedAddress;

#if KDE_IS_VERSION(3,1,90)
        formattedAddress = (*addrIt).formattedAddress().stripWhiteSpace();
#else
        if ( !(*addrIt).street().isEmpty() )
          formattedAddress += (*addrIt).street() + "\n";

        if ( !(*addrIt).postOfficeBox().isEmpty() )
          formattedAddress += (*addrIt).postOfficeBox() + "\n";

        formattedAddress += (*addrIt).locality() + QString::fromLatin1(" ") + (*addrIt).region();

        if ( !(*addrIt).postalCode().isEmpty() )
          formattedAddress += QString::fromLatin1(", ") + (*addrIt).postalCode();

        formattedAddress += "\n";

        if ( !(*addrIt).country().isEmpty() )
          formattedAddress += (*addrIt).country() + "\n";

        formattedAddress += (*addrIt).extended();
#endif

        formattedAddress = formattedAddress.replace( '\n', "<br>" );

        dynamicPart += rowFmtStr
          .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
          .arg( formattedAddress );
      } else {
        dynamicPart += rowFmtStr
          .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
          .arg( (*addrIt).label().replace( '\n', "<br>" ) );
      }
    }
  }

  QString notes;
  if ( !mAddressee.note().isEmpty() ) {
    notes = QString::fromLatin1(
      "<tr>"
      "<td align=\"right\" valign=\"top\" width=\"30%\"><b>%1:</b></td>"  // note label
      "<td align=\"left\" valign=\"top\">%2</td>"  // note
      "</tr>" ).arg( i18n( "Notes" ) ).arg( mAddressee.note().replace( '\n', "<br>" ) );
  }

  QString role, organization;
  role = mAddressee.role().isEmpty() ? 
	QString::null : rowFmtStr.arg(QString::null).arg(mAddressee.role());
  organization = mAddressee.organization().isEmpty() ?
	QString::null : rowFmtStr.arg(QString::null).arg(mAddressee.organization());

  QString strAddr = QString::fromLatin1(
  "<html>"
  "<body text=\"%1\" bgcolor=\"%2\">" // text and background color
  "<table width=\"100%\">"
  "<tr>"
  "<td align=\"right\" valign=\"top\" width=\"30%\">"
  "<img src=\"myimage\" width=\"50\" height=\"70\">"
  "</td>"
  "<td align=\"left\" width=\"70%\"><font size=\"+2\"><b>%3</b></font></td>"  // name
  "</tr>"
  "%4"	// role
  "%5"	// organization
  "<tr><td></td></tr>"
  "%6"  // dynamic part
  "%7"  // notes
  "</table>"
  "</body>"
  "</html>")
   .arg( KGlobalSettings::textColor().name() )
   .arg( KGlobalSettings::baseColor().name() )
   .arg( name )
   .arg( role )
   .arg( organization )
   .arg( dynamicPart )
   .arg( notes )
   ;

  KABC::Picture picture = mAddressee.photo();
  if ( picture.isIntern() && !picture.data().isNull() )
    QMimeSourceFactory::defaultFactory()->setImage( "myimage", picture.data() );
  else {
    if ( !picture.url().isEmpty() ) {
      if ( mImageData.count() > 0 )
        QMimeSourceFactory::defaultFactory()->setImage( "myimage", mImageData );
      else {
        mImageJob = KIO::get( KURL( picture.url() ), false, false );
        connect( mImageJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
                 this, SLOT( data( KIO::Job*, const QByteArray& ) ) );
        connect( mImageJob, SIGNAL( result( KIO::Job* ) ),
                 this, SLOT( result( KIO::Job* ) ) );
      }
    } else {
      QMimeSourceFactory::defaultFactory()->setPixmap( "myimage",
        KGlobal::iconLoader()->loadIcon( "identity", KIcon::Desktop, 128 ) );
    }
  }

  // at last display it...
  setText( strAddr );
}

KABC::Addressee AddresseeView::addressee() const
{
  return mAddressee;
}

void AddresseeView::urlClicked( const QString &url )
{
  kapp->invokeBrowser( url );
}

void AddresseeView::emailClicked( const QString &email )
{
  kapp->invokeMailer( email, QString::null );
}

void AddresseeView::phoneNumberClicked( const QString &number )
{
  KConfig config( "kaddressbookrc" );
  config.setGroup( "General" );
  QString commandLine = config.readEntry( "PhoneHookApplication" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  commandLine.replace( "%N", number );
  KRun::runCommand( commandLine );
}

void AddresseeView::faxNumberClicked( const QString &number )
{
  KConfig config( "kaddressbookrc" );
  config.setGroup( "General" );
  QString commandLine = config.readEntry( "FaxHookApplication", "kdeprintfax --phone %N" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  commandLine.replace( "%N", number );
  KRun::runCommand( commandLine );
}

QPopupMenu *AddresseeView::createPopupMenu( const QPoint& )
{
  QPopupMenu *menu = new QPopupMenu( this );
  mActionShowBirthday->plug( menu );
  mActionShowAddresses->plug( menu );
  mActionShowEmails->plug( menu );
  mActionShowPhones->plug( menu );
  mActionShowURLs->plug( menu );

  return menu;
}

void AddresseeView::slotMailClicked( const QString&, const QString &email )
{
  emailClicked( email );
}

void AddresseeView::slotUrlClicked( const QString &url )
{
  if ( url.startsWith( "phone:" ) )
    phoneNumberClicked( strippedNumber( url.mid( 8 ) ) );
  else if ( url.startsWith( "fax:" ) )
    faxNumberClicked( strippedNumber( url.mid( 6 ) ) );
  else
    urlClicked( url );
}

void AddresseeView::slotHighlighted( const QString &link )
{
  if ( link.startsWith( "mailto:" ) ) {
    QString email = link.mid( 7 );

    emit emailHighlighted( email );
    emit highlightedMessage( i18n( "Send mail to <%1>" ).arg( email ) );
  } else if ( link.startsWith( "phone:" ) ) {
    QString number = link.mid( 8 );

    emit phoneNumberHighlighted( strippedNumber( number ) );
    emit highlightedMessage( i18n( "Call number %1" ).arg( number ) );
  } else if ( link.startsWith( "fax:" ) ) {
    QString number = link.mid( 6 );

    emit faxNumberHighlighted( strippedNumber( number ) );
    emit highlightedMessage( i18n( "Send fax to %1" ).arg( number ) );
  } else if ( link.startsWith( "http:" ) ) {
    emit urlHighlighted( link );
    emit highlightedMessage( i18n( "Open URL %1" ).arg( link ) );
  } else
    emit highlightedMessage( "" );
}

void AddresseeView::configChanged()
{
  save();
  updateView();
}

void AddresseeView::data( KIO::Job*, const QByteArray &d )
{
  unsigned int oldSize = mImageData.size();
  mImageData.resize( oldSize + d.size() );
  memcpy( mImageData.data() + oldSize, d.data(), d.size() );
}

void AddresseeView::result( KIO::Job *job )
{
  if ( !job->error() ) {
    mImageJob = 0;
    updateView();
  }
}

void AddresseeView::load()
{
  mConfig->setGroup( "AddresseeViewSettings" );
  mActionShowBirthday->setChecked( mConfig->readBoolEntry( "ShowBirthday", false ) );
  mActionShowAddresses->setChecked( mConfig->readBoolEntry( "ShowAddresses", true ) );
  mActionShowEmails->setChecked( mConfig->readBoolEntry( "ShowEmails", true ) );
  mActionShowPhones->setChecked( mConfig->readBoolEntry( "ShowPhones", true ) );
  mActionShowURLs->setChecked( mConfig->readBoolEntry( "ShowURLs", true ) );
}

void AddresseeView::save()
{
  mConfig->setGroup( "AddresseeViewSettings" );
  mConfig->writeEntry( "ShowBirthday", mActionShowBirthday->isChecked() );
  mConfig->writeEntry( "ShowAddresses", mActionShowAddresses->isChecked() );
  mConfig->writeEntry( "ShowEmails", mActionShowEmails->isChecked() );
  mConfig->writeEntry( "ShowPhones", mActionShowPhones->isChecked() );
  mConfig->writeEntry( "ShowURLs", mActionShowURLs->isChecked() );
  mConfig->sync();
}

QString AddresseeView::strippedNumber( const QString &number )
{
  QString retval;

  for ( uint i = 0; i < number.length(); ++i )
    if ( number[ i ].isDigit() )
      retval.append( number[ i ] );

  return retval;
}

#include "addresseeview.moc"
