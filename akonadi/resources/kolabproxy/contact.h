/*
    This file is part of libkabc and/or kaddressbook.
    Copyright (c) 2002 - 2004 Klarälvdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>

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

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef KOLABCONTACT_H
#define KOLABCONTACT_H

#include "kolabbase.h"
#include <qimage.h>

namespace KABC {
  class Addressee;
  class AddressBook;
  class ResourceKolab;
  class Picture;
  class Sound;
}

namespace Kolab {

class Contact : public KolabBase {
public:
  struct PhoneNumber {
  public:
    QString type;
    QString number;
  };

  struct Address {
  public:
    Address() : kdeAddressType( -1 )
    {
    }
    int kdeAddressType; // KABC::Address::Type
    QString type;       // kolab-compliant address type: home, work or other
    QString street;
    QString pobox;
    QString locality;
    QString region;
    QString postalCode;
    QString country;
  };

  explicit Contact( const KABC::Addressee* address, KABC::AddressBook* addressBook );
  Contact( const QString& xml );
  ~Contact();

  void saveTo( KABC::Addressee* address );

  QString type() const { return "Contact"; }

  void setGivenName( const QString& name );
  QString givenName() const;

  void setMiddleNames( const QString& names );
  QString middleNames() const;

  void setLastName( const QString& name );
  QString lastName() const;

  void setFullName( const QString& name );
  QString fullName() const;

  void setInitials( const QString& initials );
  QString initials() const;

  void setPrefix( const QString& prefix );
  QString prefix() const;

  void setSuffix( const QString& suffix );
  QString suffix() const;

  void setRole( const QString& role );
  QString role() const;

  void setFreeBusyUrl( const QString& fbUrl );
  QString freeBusyUrl() const;

  void setOrganization( const QString& organization );
  QString organization() const;

  void setWebPage( const QString& url );
  QString webPage() const;

  void setIMAddress( const QString& imAddress );
  QString imAddress() const;

  void setDepartment( const QString& department );
  QString department() const;

  void setOfficeLocation( const QString& location );
  QString officeLocation() const;

  void setProfession( const QString& profession );
  QString profession() const;

  void setTitle( const QString& title );
  QString title() const;

  void setManagerName( const QString& name );
  QString managerName() const;

  void setAssistant( const QString& name );
  QString assistant() const;

  void setNickName( const QString& name );
  QString nickName() const;

  void setSpouseName( const QString& name );
  QString spouseName() const;

  void setBirthday( const QDate& date );
  QDate birthday() const;

  void setAnniversary( const QDate& date );
  QDate anniversary() const;

  void setPicture( const QImage& image) { mPicture = image; }
  QString pictureAttachmentName() const { return mPictureAttachmentName; }
  QImage picture() const { return mPicture; }

  void setLogo( const QImage& image ) { mLogo = image; }
  QString logoAttachmentName() const { return mLogoAttachmentName; }
  QImage logo() const { return mLogo; }

  void setSound( const QByteArray& sound ) { mSound = sound; }
  QString soundAttachmentName() const { return mSoundAttachmentName; }
  QByteArray sound() const { return mSound; }

  void setChildren( const QString& children );
  QString children() const;

  void setGender( const QString& gender );
  QString gender() const;

  void setLanguage( const QString& language );
  QString language() const;

  void addPhoneNumber( const PhoneNumber& number );
  QList<PhoneNumber>& phoneNumbers();
  const QList<PhoneNumber>& phoneNumbers() const;

  void addEmail( const Email& email );
  QList<Email>& emails();
  const QList<Email>& emails() const;

  QString fullEmail() const;

  void addAddress( const Address& address );
  QList<Address>& addresses();
  const QList<Address>& addresses() const;

  // which address is preferred: home or business or other
  void setPreferredAddress( const QString& address );
  QString preferredAddress() const;

  float latitude() const { return mLatitude; }
  void setLatitude( float latitude ) { mLatitude = latitude; }

  float longitude() const { return mLongitude; }
  void setLongitude( float longitude ) { mLongitude = longitude; }

  // Load the attributes of this class
  bool loadAttribute( QDomElement& );

  // Save the attributes of this class
  bool saveAttributes( QDomElement& ) const;

  // Load this note by reading the XML file
  bool loadXML( const QDomDocument& xml );

  // Serialize this note to an XML string
  QString saveXML() const;

protected:
  void setFields( const KABC::Addressee*, KABC::AddressBook* );

private:
  bool loadNameAttribute( QDomElement& element );
  void saveNameAttribute( QDomElement& element ) const;

  bool loadPhoneAttribute( QDomElement& element );
  void savePhoneAttributes( QDomElement& element ) const;

  void saveEmailAttributes( QDomElement& element ) const;

  bool loadAddressAttribute( QDomElement& element );
  void saveAddressAttributes( QDomElement& element ) const;

  void loadCustomAttributes( QDomElement& element );
  void saveCustomAttributes( QDomElement& element ) const;

  QImage loadPictureFromAddressee( const KABC::Picture& picture );

  QByteArray loadSoundFromAddressee( const KABC::Sound& sound );

  QString productID() const;

  QString mGivenName;
  QString mMiddleNames;
  QString mLastName;
  QString mFullName;
  QString mInitials;
  QString mPrefix;
  QString mSuffix;
  QString mRole;
  QString mFreeBusyUrl;
  QString mOrganization;
  QString mWebPage;
  QString mIMAddress;
  QString mDepartment;
  QString mOfficeLocation;
  QString mProfession;
  QString mTitle;
  QString mManagerName;
  QString mAssistant;
  QString mNickName;
  QString mSpouseName;
  QDate mBirthday;
  QDate mAnniversary;
  QImage mPicture;
  QImage mLogo;
  QByteArray mSound;
  QString mPictureAttachmentName;
  QString mLogoAttachmentName;
  QString mSoundAttachmentName;
  QString mChildren;
  QString mGender;
  QString mLanguage;
  QList<PhoneNumber> mPhoneNumbers;
  QList<Email> mEmails;
  QString mFullEmail;
  QList<Address> mAddresses;
  QString mPreferredAddress;
  float mLatitude;
  float mLongitude;
  bool mHasGeo;
  struct Custom {
    QString app;
    QString name;
    QString value;
  };
  QList<Custom> mCustomList;
};

}

#endif // KOLABCONTACT_H
