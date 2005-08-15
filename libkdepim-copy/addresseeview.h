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

#ifndef KPIM_ADDRESSEEVIEW_H
#define KPIM_ADDRESSEEVIEW_H

#include <qcstring.h>

#include <kabc/addressee.h>
#include <ktextbrowser.h>
#include <kimproxy.h>
#include <kdepimmacros.h>

namespace KIO {
class Job;
}
class KToggleAction;

class QPopupMenu;


namespace KPIM {


class KDE_EXPORT AddresseeView : public KTextBrowser
{
  Q_OBJECT
  public:
    /**
      Constructor.
 
      @param config The config object where the settings are stored
                    which fields will be shown.
     */
    AddresseeView( QWidget *parent = 0, const char *name = 0,
                   KConfig *config = 0 );

    ~AddresseeView();

    /**
      Sets the addressee object. The addressee is displayed immediately.

      @param addr The addressee object.
     */
    void setAddressee( const KABC::Addressee& addr );

    /**
      Returns the current addressee object.
     */
    KABC::Addressee addressee() const;


    /**
      This enums are used by enableLinks to set which kind of links shall
      be enabled.
     */
    enum LinkMask {
      NoLinks = 0,
      AddressLinks = 1,
      EmailLinks = 2,
      PhoneLinks = 4,
      URLLinks = 8,
      IMLinks = 16,
      DefaultLinks = AddressLinks | EmailLinks | PhoneLinks | URLLinks | IMLinks
    };

    /**
      Sets which parts of the contact shall be presented as links.
      The mask can be OR'ed LinkMask. By default all links are enabled.
     */
    void enableLinks( int linkMask );

    /**
      This enums are used by vCardAsHTML to decide which fields shall be
      shown.
     */
    enum FieldMask {
      NoFields = 0,
      BirthdayFields = 1,
      AddressFields = 2,
      EmailFields = 4,
      PhoneFields = 8,
      URLFields = 16,
      IMFields = 32,
      CustomFields = 64,
      DefaultFields = AddressFields | EmailFields | PhoneFields | URLFields
    };

    /**
      Returns the HTML representation of a contact.
      The HTML code looks like
        <div>
        <table>
        ...
        </table>
        </div>

      @param addr The addressee object.
      @param linkMask The mask for which parts of the contact will
                      be displayed as links.
                      The links looks like this:
                        "addr://<addr id>" for addresses
                        "mailto:<email address>" for emails
                        "phone://<phone number>" for phone numbers
                        "http://<url>" for urls
                        "im:<im addrss>" for instant messaging addresses
                        "sms://<phone number>" for sending a sms
      @param internalLoading If true, the loading of internal pictures is done automatically.
      @param fieldMask The mask for which fields of the contact will
                       be displayed.
     */
    static QString vCardAsHTML( const KABC::Addressee& addr, ::KIMProxy *proxy, LinkMask linkMask = DefaultLinks,
                                bool internalLoading = true, FieldMask fieldMask = DefaultFields );

    /**
     * Encodes a QPixmap as a PNG into a data: URL (rfc2397), readable by the data kio protocol
     * @param pixmap the pixmap to encode
     * @return a data: URL
     */
    static QString pixmapAsDataUrl( const QPixmap& pixmap );

  signals:
    void urlHighlighted( const QString &url );
    void emailHighlighted( const QString &email );
    void phoneNumberHighlighted( const QString &number );
    void faxNumberHighlighted( const QString &number );

    void highlightedMessage( const QString &message );

    void addressClicked( const QString &uid );

  protected:
    virtual void urlClicked( const QString &url );
    virtual void emailClicked( const QString &mail );
    virtual void phoneNumberClicked( const QString &number );
    virtual void smsTextClicked( const QString &number );
    virtual void sendSMS( const QString &number, const QString &msg );
    virtual void faxNumberClicked( const QString &number );
    virtual void imAddressClicked();

    virtual QPopupMenu *createPopupMenu( const QPoint& );

  private slots:
    void slotMailClicked( const QString&, const QString& );
    void slotUrlClicked( const QString& );
    void slotHighlighted( const QString& );
    void slotPresenceChanged( const QString & );
    void slotPresenceInfoExpired();
    void configChanged();

    void data( KIO::Job*, const QByteArray& );
    void result( KIO::Job* );

  private:
    void load();
    void save();

    void updateView();

    QString strippedNumber( const QString &number );

    KConfig *mConfig;
    bool mDefaultConfig;

    QByteArray mImageData;
    KIO::Job *mImageJob;

    KToggleAction *mActionShowBirthday;
    KToggleAction *mActionShowAddresses;
    KToggleAction *mActionShowEmails;
    KToggleAction *mActionShowPhones;
    KToggleAction *mActionShowURLs;
    KToggleAction *mActionShowIMAddresses;
    KToggleAction *mActionShowCustomFields;

    KABC::Addressee mAddressee;
    int mLinkMask;

    class AddresseeViewPrivate;
    AddresseeViewPrivate *d;
    ::KIMProxy *mKIMProxy;
};

}

#endif
