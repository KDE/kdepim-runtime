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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QBuffer>
#include <QImage>
#include <QMenu>
#include <QPixmap>
#include <QTextStream>
#include <QContextMenuEvent>

#include <kabc/address.h>
#include <kabc/addressee.h>
#include <kabc/phonenumber.h>
#include <kconfig.h>
#include <kcolorscheme.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kcodecs.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kurl.h>
#include <kstringhandler.h>
#include <ktemporaryfile.h>

#include <kdebug.h>
#include <ktoolinvocation.h>

#include "addresseeview.h"
#include "sendsmsdialog.h"
#include <Q3MimeSourceFactory>
#include <ktoggleaction.h>
using namespace KPIM;

AddresseeView::AddresseeView( QWidget *parent, KConfig *config )
  : KTextBrowser( parent ), mDefaultConfig( false ), mImageJob( 0 ),
    mLinkMask( AddressLinks | EmailLinks | PhoneLinks | URLLinks | IMLinks | CustomFields )
{
  setWordWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );
  setHorizontalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
  setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

  connect( this, SIGNAL( mailClick( const QString&, const QString& ) ),
           this, SLOT( slotMailClicked( const QString&, const QString& ) ) );
  connect( this, SIGNAL( urlClick( const QString& ) ),
           this, SLOT( slotUrlClicked( const QString& ) ) );
  connect( this, SIGNAL( highlighted( const QString& ) ),
           this, SLOT( slotHighlighted( const QString& ) ) );

  setNotifyClick( true );

  mActionShowBirthday = new KToggleAction( i18n( "Show Birthday" ), this );
  mActionShowBirthday->setCheckedState( KGuiItem( i18n( "Hide Birthday" ) ) );
  mActionShowAddresses = new KToggleAction( i18n( "Show Postal Addresses" ), this );
  mActionShowAddresses->setCheckedState( KGuiItem( i18n( "Hide Postal Addresses" ) ) );
  mActionShowEmails = new KToggleAction( i18n( "Show Email Addresses" ), this );
  mActionShowEmails->setCheckedState( KGuiItem( i18n( "Hide Email Addresses" ) ) );
  mActionShowPhones = new KToggleAction( i18n( "Show Telephone Numbers" ), this );
  mActionShowPhones->setCheckedState( KGuiItem( i18n( "Hide Telephone Numbers" ) ) );
  mActionShowURLs = new KToggleAction( i18n( "Show Web Pages (URLs)" ), this );
  mActionShowURLs->setCheckedState( KGuiItem( i18n( "Hide Web Pages (URLs)" ) ) );
  mActionShowIMAddresses = new KToggleAction( i18n( "Show Instant Messaging Addresses" ), this );
  mActionShowIMAddresses->setCheckedState( KGuiItem( i18n( "Hide Instant Messaging Addresses" ) ) );
  mActionShowCustomFields = new KToggleAction( i18n( "Show Custom Fields" ), this );
  mActionShowCustomFields->setCheckedState( KGuiItem( i18n( "Hide Custom Fields" ) ) );

  connect( mActionShowBirthday, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowAddresses, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowEmails, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowPhones, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowURLs, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowIMAddresses, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowCustomFields, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );

  if ( !config ) {
    mConfig = new KConfig( "kaddressbookrc" );
    mDefaultConfig = true;
  } else
    mConfig = config;

  load();

  // set up IMProxy to display contacts' IM presence and make connections to keep the display live
  mKIMProxy = ::KIMProxy::instance();
  connect( mKIMProxy, SIGNAL( sigContactPresenceChanged( const QString& ) ),
           this, SLOT( slotPresenceChanged( const QString& ) ) );
  connect( mKIMProxy, SIGNAL( sigPresenceInfoExpired() ),
           this, SLOT( slotPresenceInfoExpired() ) );
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
  delete mActionShowIMAddresses;
  delete mActionShowCustomFields;

  mKIMProxy = 0;
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

void AddresseeView::enableLinks( int linkMask )
{
  mLinkMask = linkMask;
}

QString AddresseeView::vCardAsHTML( const KABC::Addressee& addr, ::KIMProxy*, LinkMask linkMask,
                                    bool internalLoading, FieldMask fieldMask )
{
  QString image = QString( "contact_%1_image" ).arg( addr.uid() );

  // Style strings from Gentix; this is just an initial version.
  //
  // These will be substituted into various HTML strings with .arg().
  // Search for @STYLE@ to find where. Note how we use %1 as a
  // placeholder where we fill in something else (in this case,
  // the global background color).
  //
  const QString backgroundColor = KColorScheme( QPalette::Active, KColorScheme::View ).background( KColorScheme::AlternateBackground ).color().name();
  QString cellStyle = QString::fromLatin1(
        "style=\""
        "padding-right: 2px; "
        "border-right: #000 dashed 1px; "
        "background: %1;\"").arg(backgroundColor);
  const QString backgroundColor2 = KColorScheme( QPalette::Active, KColorScheme::View ).background().color().name();
  QString cellStyle2 = QString::fromLatin1(
        "style=\""
        "padding-left: 2px; "
        "background: %1;\"").arg(backgroundColor2);
  QString tableStyle = QString::fromLatin1(
        "style=\""
        "border: solid 1px; "
        "margin: 0em;\"");

  // We'll be building a table to display the vCard in.
  // Each row of the table will be built using this string for its HTML.
  //
  QString rowFmtStr = QString::fromLatin1(
        "<tr>"
        "<td align=\"right\" valign=\"top\" width=\"30%\" "); // Tag unclosed
  rowFmtStr.append( cellStyle );
  rowFmtStr.append( QString::fromLatin1(
	">" // Close tag
        "<b>%1</b>"
        "</td>"
        "<td align=\"left\" valign=\"top\" width=\"70%\" ") ); // Tag unclosed
  rowFmtStr.append( cellStyle2 );
  rowFmtStr.append( QString::fromLatin1(
	">" // Close tag
        "%2"
        "</td>"
        "</tr>\n"
        ) );

  // Build the table's rows here
  QString dynamicPart;


  if ( !internalLoading ) {
    KABC::Picture pic = addr.photo();
    if ( pic.isIntern() && !pic.data().isNull() ) {
      image = pixmapAsDataUrl( QPixmap::fromImage( pic.data() ) );
    } else if ( !pic.url().isEmpty() ) {
      image = (pic.url().startsWith( "http://" ) || pic.url().startsWith( "https://" ) ? pic.url() : "http://" + pic.url());
    } else {
      image = "file:" + KIconLoader::global()->iconPath( "personal", KIconLoader::Desktop );
    }
  }

  if ( fieldMask & BirthdayFields ) {
    QDate date = addr.birthday().date();

    if ( date.isValid() )
      dynamicPart += rowFmtStr
        .arg( KABC::Addressee::birthdayLabel() )
        .arg( KGlobal::locale()->formatDate( date, KLocale::ShortDate ) );
  }

  if ( fieldMask & PhoneFields ) {
    KABC::PhoneNumber::List phones = addr.phoneNumbers();
    KABC::PhoneNumber::List::ConstIterator phoneIt;
    for ( phoneIt = phones.begin(); phoneIt != phones.end(); ++phoneIt ) {
      QString number = (*phoneIt).number();

      QString url;
      if ( (*phoneIt).type() & KABC::PhoneNumber::Fax )
        url = QString::fromLatin1( "fax:" ) + number;
      else
        url = QString::fromLatin1( "phone:" ) + number;

      if ( linkMask & PhoneLinks ) {
        QString smsURL;
        if ( (*phoneIt).type() & KABC::PhoneNumber::Cell )
          smsURL = QString(" (<a href=\"sms:%1\">%2</a>)" ).arg( number ).arg( i18n( "SMS") );

        dynamicPart += rowFmtStr
          .arg( KABC::PhoneNumber::typeLabel( (*phoneIt).type() ).replace( " ", "&nbsp;" ) )
          .arg( QString::fromLatin1( "<a href=\"%1\">%2</a>%3" ).arg( url ).arg( number ).arg( smsURL ) );
      } else {
        dynamicPart += rowFmtStr
          .arg( KABC::PhoneNumber::typeLabel( (*phoneIt).type() ).replace( " ", "&nbsp;" ) )
          .arg( number );
      }
    }
  }

  if ( fieldMask & EmailFields ) {
    QStringList emails = addr.emails();
    QStringList::ConstIterator emailIt;
    QString type = i18n( "Email" );
    for ( emailIt = emails.begin(); emailIt != emails.end(); ++emailIt ) {
      QByteArray fullEmail = KUrl::toPercentEncoding( addr.fullEmail( *emailIt ) );

      if ( linkMask & EmailLinks ) {
        dynamicPart += rowFmtStr.arg( type )
          .arg( QString::fromLatin1( "<a href=\"mailto:%1\">%2</a>" )
          .arg( fullEmail, *emailIt ) );
      } else {
        dynamicPart += rowFmtStr.arg( type ).arg( *emailIt );
      }
    }
  }

  if ( fieldMask & URLFields ) {
    if ( !addr.url().url().isEmpty() ) {
      QString url;
      if ( linkMask & URLLinks ) {
        url = (addr.url().url().startsWith( "http://" ) || addr.url().url().startsWith( "https://" ) ? addr.url().url() :
          "http://" + addr.url().url());
        url = KStringHandler::tagUrls( url );
      } else {
        url = addr.url().url();
      }
      dynamicPart += rowFmtStr.arg( i18n("Homepage") ).arg( url );
    }

    QString blog = addr.custom( "KADDRESSBOOK", "BlogFeed" );
    if ( !blog.isEmpty() ) {
      if ( linkMask & URLLinks ) {
        blog = KStringHandler::tagUrls( blog );
      }
      dynamicPart += rowFmtStr.arg( i18n("Blog Feed") ).arg( blog );
    }
  }

  if ( fieldMask & AddressFields ) {
    KABC::Address::List addresses = addr.addresses();
    KABC::Address::List::ConstIterator addrIt;
    for ( addrIt = addresses.begin(); addrIt != addresses.end(); ++addrIt ) {
      if ( (*addrIt).label().isEmpty() ) {
        QString formattedAddress;

        formattedAddress = (*addrIt).formattedAddress().trimmed();

        formattedAddress = formattedAddress.replace( '\n', "<br>" );

        QString link = "<a href=\"addr:" + (*addrIt).id() + "\">" +
                       formattedAddress + "</a>";

        if ( linkMask & AddressLinks ) {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( link );
        } else {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( formattedAddress );
        }
      } else {
        QString link = "<a href=\"addr:" + (*addrIt).id() + "\">" +
                       (*addrIt).label().replace( '\n', "<br>" ) + "</a>";

        if ( linkMask & AddressLinks ) {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( link );
        } else {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( (*addrIt).label().replace( '\n', "<br>" ) );
        }
      }
    }
  }

  QString notes;
  if ( !addr.note().isEmpty() ) {
    // @STYLE@ - substitute the cell style in first, and append
    // the data afterwards (keeps us safe from possible % signs
    // in either one).
    notes = rowFmtStr.arg( i18n( "Notes" ) ).arg( addr.note().replace( '\n', "<br>" ) ) ;
  }

  QString customData;
  if ( fieldMask & CustomFields ) {
    static QMap<QString, QString> titleMap;
    if ( titleMap.isEmpty() ) {
      titleMap.insert( "Department", i18n( "Department" ) );
      titleMap.insert( "Profession", i18n( "Profession" ) );
      titleMap.insert( "AssistantsName", i18n( "Assistant's Name" ) );
      titleMap.insert( "ManagersName", i18n( "Manager's Name" ) );
      titleMap.insert( "SpousesName", i18n( "Partner's Name" ) );
      titleMap.insert( "Office", i18n( "Office" ) );
      titleMap.insert( "IMAddress", i18n( "IM Address" ) );
      titleMap.insert( "Anniversary", i18n( "Anniversary" ) );
    }

    if ( !addr.customs().empty() ) {
      QStringList customs = addr.customs();
      QStringList::Iterator it( customs.begin() );
      const QStringList::Iterator endIt( customs.end() );
      for ( ; it != endIt; ++it ) {
        QString customEntry = *it;
        if ( customEntry.startsWith ( "KADDRESSBOOK-" ) ) {
          customEntry.remove( "KADDRESSBOOK-X-" );
          customEntry.remove( "KADDRESSBOOK-" );

          int pos = customEntry.indexOf( ':' );
          QString key = customEntry.left( pos );
          const QString value = customEntry.mid( pos + 1 );

          // blog is handled separated
          if ( key == "BlogFeed" )
            continue;

          const QMap<QString, QString>::ConstIterator keyIt = titleMap.find( key );
          if ( keyIt != titleMap.end() )
            key = keyIt.value();

          customData += rowFmtStr.arg( key ).arg( value ) ;
        }
      }
    }
  }

  QString name( addr.realName() );
  QString role( addr.role() );
  QString organization( addr.organization() );
/*
  if ( proxy && (fieldMask & IMFields) ) {
    if ( proxy->isPresent( addr.uid() ) && proxy->presenceNumeric( addr.uid() ) > 0 ) {
      // set image source to either a QMimeSourceFactory key or a data:/ URL
      QString imgSrc = pixmapAsDataUrl( proxy->presenceIcon( addr.uid() ) );

      // make the status a link, if required
      QString imStatus;
      if ( linkMask & IMLinks )
        imStatus = QString::fromLatin1( "<a href=\"im:\"><img src=\"%1\"> (%2)</a>" );
      else
        imStatus = QString::fromLatin1( "<img src=\"%1\"> (%2)" );

      // append our status to the rest of the dynamic part of the addressee
      dynamicPart += rowFmtStr
              .arg( i18n( "Presence" ) )
              .arg( imStatus
                        .arg( imgSrc )
                        .arg( proxy->presenceString( addr.uid() ) )
                  );
    }
  }
*/
  // @STYLE@ - construct the string by parts, substituting in
  // the styles first. There are lots of appends, but we need to
  // do it this way to avoid cases where the substituted string
  // contains %1 and the like.
  //
  QString strAddr = QString::fromLatin1(
    "<div align=\"center\">"
    "<table cellpadding=\"1\" cellspacing=\"0\" %1>"
    "<tr>").arg(tableStyle);

  strAddr.append( QString::fromLatin1(
    "<td align=\"right\" valign=\"top\" width=\"30%\" rowspan=\"3\" %2>")
    .arg( cellStyle ) );
  strAddr.append( QString::fromLatin1(
    "<img src=\"%1\" width=\"75\" height=\"105\" vspace=\"1\">" // image
    "</td>")
    .arg( image ) );
  strAddr.append( QString::fromLatin1(
    "<td align=\"left\" width=\"70%\" %2>")
    .arg( cellStyle2 ) );
  strAddr.append( QString::fromLatin1(
    "<font size=\"+2\"><b>%2</b></font></td>"  // name
    "</tr>")
    .arg( name ) );
  strAddr.append( QString::fromLatin1(
    "<tr>"
    "<td align=\"left\" width=\"70%\" %2>")
    .arg( cellStyle2 ) );
  strAddr.append( QString::fromLatin1(
    "%3</td>"  // role
    "</tr>")
    .arg( role ) );
  strAddr.append( QString::fromLatin1(
    "<tr>"
    "<td align=\"left\" width=\"70%\" %2>")
    .arg( cellStyle2 ) );
  strAddr.append( QString::fromLatin1(
    "%4</td>"  // organization
    "</tr>")
    .arg( organization ) );
  strAddr.append( QString::fromLatin1(
    "<tr><td %2>")
    .arg( cellStyle ) );
  strAddr.append( QString::fromLatin1(
    "&nbsp;</td><td %2>&nbsp;</td></tr>")
    .arg( cellStyle2 ) );
  strAddr.append( dynamicPart );
  strAddr.append( notes );
  strAddr.append( customData );
  strAddr.append( QString::fromLatin1( "</table></div>\n" ) );

  return strAddr;
}

QString AddresseeView::pixmapAsDataUrl( const QPixmap& pixmap )
{
  QByteArray ba;
  QBuffer buffer( &ba, 0 );
  buffer.open( QIODevice::WriteOnly );
  pixmap.save( &buffer, "PNG" );
  QString encoded( "data:image/png;base64," );
  encoded.append( ba.toBase64() );
  return encoded;
}

void AddresseeView::updateView()
{
  // clear view
  setPlainText( QString() );

  if ( mAddressee.isEmpty() )
    return;

  if ( mImageJob ) {
    mImageJob->kill();
    mImageJob = 0;

    mImageData.truncate( 0 );
  }

  int fieldMask = NoFields;
  if ( mActionShowBirthday->isChecked() )
    fieldMask |= ( FieldMask )BirthdayFields;
  if ( mActionShowAddresses->isChecked() )
    fieldMask |= AddressFields;
  if ( mActionShowEmails->isChecked() )
    fieldMask |= EmailFields;
  if ( mActionShowPhones->isChecked() )
    fieldMask |= PhoneFields;
  if ( mActionShowURLs->isChecked() )
    fieldMask |= URLFields;
  if ( mActionShowIMAddresses->isChecked() )
    fieldMask |= IMFields;
  if ( mActionShowCustomFields->isChecked() )
    fieldMask |= CustomFields;

  QString strAddr = vCardAsHTML( mAddressee, mKIMProxy, (LinkMask)mLinkMask,
                                 true, (FieldMask)fieldMask );

  strAddr = QString::fromLatin1(
    "<html>"
    "<body text=\"%1\" bgcolor=\"%2\">" // text and background color
    "%3" // dynamic part
    "</body>"
    "</html>" )
     .arg( KColorScheme( QPalette::Active, KColorScheme::View ).foreground().color().name() )
     .arg( KColorScheme( QPalette::Active, KColorScheme::View ).background().color().name() )
     .arg( strAddr );

  QString imageURL = QString( "contact_%1_image" ).arg( mAddressee.uid() );

  KABC::Picture picture = mAddressee.photo();
  if ( picture.isIntern() && !picture.data().isNull() )
    Q3MimeSourceFactory::defaultFactory()->setImage( imageURL, picture.data() );
  else {
    if ( !picture.url().isEmpty() ) {
      if ( mImageData.count() > 0 )
        Q3MimeSourceFactory::defaultFactory()->setImage( imageURL, QImage::fromData( mImageData ) );
      else {
        mImageJob = KIO::get( KUrl( picture.url() ), false, false );
        connect( mImageJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
                 this, SLOT( data( KIO::Job*, const QByteArray& ) ) );
        connect( mImageJob, SIGNAL( result( KJob* ) ),
                 this, SLOT( result( KJob* ) ) );
      }
    } else {
      Q3MimeSourceFactory::defaultFactory()->setPixmap( imageURL,
        KIconLoader::global()->loadIcon( "personal", KIconLoader::Desktop, 128 ) );
    }
  }

  // at last display it...
  setHtml( strAddr );
}

KABC::Addressee AddresseeView::addressee() const
{
  return mAddressee;
}

void AddresseeView::urlClicked( const QString &url )
{
  KToolInvocation::invokeBrowser( url );
}

void AddresseeView::emailClicked( const QString &email )
{
  if ( email.startsWith( "mailto:" ) )
    KToolInvocation::invokeMailer( email.mid( 7 ), QString() );
  else
    KToolInvocation::invokeMailer( email, QString() );
}

void AddresseeView::phoneNumberClicked( const QString &number )
{
  KConfig _config( "kaddressbookrc" );
  KConfigGroup config(&_config, "General" );
  QString commandLine = config.readEntry( "PhoneHookApplication" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  commandLine.replace( "%N", number );
  KRun::runCommand( commandLine, topLevelWidget());
}

void AddresseeView::smsTextClicked( const QString &number )
{
  KConfig _config( "kaddressbookrc" );
  KConfigGroup config(&_config, "General" );
  QString commandLine = config.readEntry( "SMSHookApplication" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  SendSMSDialog dlg( mAddressee.realName(), this );

  if ( dlg.exec() )
    sendSMS ( number, dlg.text() );
}

void AddresseeView::sendSMS( const QString &number, const QString &text )
{
  KConfig _config( "kaddressbookrc" );
  KConfigGroup config(&_config, "General" );
  QString commandLine = config.readEntry( "SMSHookApplication" );

  KTemporaryFile file;
  file.setAutoRemove(false);
  file.open();
  QTextStream stream ( &file );
  stream << text;
  stream.flush();

  commandLine.replace( "%N", number );
  commandLine.replace( "%F", file.fileName() );

  KRun::runCommand( commandLine, topLevelWidget());
}

void AddresseeView::faxNumberClicked( const QString &number )
{
  KConfig _config( "kaddressbookrc" );
  KConfigGroup config(&_config, "General" );
  QString commandLine = config.readEntry( "FaxHookApplication", "kdeprintfax --phone %N" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  commandLine.replace( "%N", number );
  KRun::runCommand( commandLine, topLevelWidget());
}

void AddresseeView::imAddressClicked()
{
  // mKIMProxy->chatWithContact( mAddressee.uid() );
}

void AddresseeView::contextMenuEvent(QContextMenuEvent *e)
{
  QMenu *menu = new QMenu( this );
  menu->addAction( mActionShowBirthday );
  menu->addAction( mActionShowAddresses );
  menu->addAction( mActionShowEmails );
  menu->addAction( mActionShowPhones );
  menu->addAction( mActionShowURLs );
  menu->addAction( mActionShowIMAddresses );
  menu->addAction( mActionShowCustomFields );
  menu->exec(e->globalPos());
  delete menu;
}

void AddresseeView::slotMailClicked( const QString&, const QString &email )
{
  emailClicked( email );
}

void AddresseeView::slotUrlClicked( const QString &url )
{
  if ( url.startsWith( "phone:" ) )
    phoneNumberClicked( strippedNumber( url.mid( 8 ) ) );
  else if ( url.startsWith( "sms:" ) )
    smsTextClicked( strippedNumber( url.mid( 6 ) ) );
  else if ( url.startsWith( "fax:" ) )
    faxNumberClicked( strippedNumber( url.mid( 6 ) ) );
  else if ( url.startsWith( "addr:" ) )
    emit addressClicked( url.mid( 7 ) );
  else if ( url.startsWith( "im:" ) )
    imAddressClicked();
  else
    urlClicked( url );
}

void AddresseeView::slotHighlighted( const QString &link )
{
  if ( link.startsWith( "mailto:" ) ) {
    QString email = link.mid( 7 );

    emit emailHighlighted( email );
    emit highlightedMessage( i18n( "Send mail to '%1'", email ) );
  } else if ( link.startsWith( "phone:" ) ) {
    QString number = link.mid( 8 );

    emit phoneNumberHighlighted( strippedNumber( number ) );
    emit highlightedMessage( i18n( "Call number %1", number ) );
  } else if ( link.startsWith( "fax:" ) ) {
    QString number = link.mid( 6 );

    emit faxNumberHighlighted( strippedNumber( number ) );
    emit highlightedMessage( i18n( "Send fax to %1", number ) );
  } else if ( link.startsWith( "addr:" ) ) {
    emit highlightedMessage( i18n( "Show address on map" ) );
  } else if ( link.startsWith( "sms:" ) ) {
    QString number = link.mid( 6 );
    emit highlightedMessage( i18n( "Send SMS to %1", number ) );
  } else if ( link.startsWith( "http:" ) || link.startsWith( "https:" ) ) {
    emit urlHighlighted( link );
    emit highlightedMessage( i18n( "Open URL %1", link ) );
  } else if ( link.startsWith( "im:" ) ) {
    emit highlightedMessage( i18n( "Chat with %1", mAddressee.realName() ) );
  } else
    emit highlightedMessage( "" );
}

void AddresseeView::slotPresenceChanged( const QString &uid )
{
  kDebug() <<" uid is:" << uid <<" mAddressee is:" << mAddressee.uid();
  if ( uid == mAddressee.uid() )
    updateView();
}


void AddresseeView::slotPresenceInfoExpired()
{
  updateView();
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

void AddresseeView::result( KJob *job )
{
  mImageJob = 0;

  if ( job->error() )
    mImageData.truncate( 0 );
  else
    updateView();
}

void AddresseeView::load()
{
  KConfigGroup group( mConfig, "AddresseeViewSettings" );
  mActionShowBirthday->setChecked( group.readEntry( "ShowBirthday", false ) );
  mActionShowAddresses->setChecked( group.readEntry( "ShowAddresses", true ) );
  mActionShowEmails->setChecked( group.readEntry( "ShowEmails", true ) );
  mActionShowPhones->setChecked( group.readEntry( "ShowPhones", true ) );
  mActionShowURLs->setChecked( group.readEntry( "ShowURLs", true ) );
  mActionShowIMAddresses->setChecked( group.readEntry( "ShowIMAddresses", false ) );
  mActionShowCustomFields->setChecked( group.readEntry( "ShowCustomFields", false ) );
}

void AddresseeView::save()
{
  KConfigGroup group( mConfig, "AddresseeViewSettings" );
  group.writeEntry( "ShowBirthday", mActionShowBirthday->isChecked() );
  group.writeEntry( "ShowAddresses", mActionShowAddresses->isChecked() );
  group.writeEntry( "ShowEmails", mActionShowEmails->isChecked() );
  group.writeEntry( "ShowPhones", mActionShowPhones->isChecked() );
  group.writeEntry( "ShowURLs", mActionShowURLs->isChecked() );
  group.writeEntry( "ShowIMAddresses", mActionShowIMAddresses->isChecked() );
  group.writeEntry( "ShowCustomFields", mActionShowCustomFields->isChecked() );
  mConfig->sync();
}

QString AddresseeView::strippedNumber( const QString &number )
{
  QString retval;

  for ( int i = 0; i < number.length(); ++i ) {
    QChar c = number[ i ];
    if ( c.isDigit() || c == '*' || c == '#' || c == '+' && i == 0 )
      retval.append( c );
  }

  return retval;
}

#include "addresseeview.moc"
