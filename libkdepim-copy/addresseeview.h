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

#ifndef KPIM_ADDRESSEEVIEW_H
#define KPIM_ADDRESSEEVIEW_H

#include <qcstring.h>

#include <kabc/addressee.h>
#include <ktextbrowser.h>

namespace KIO {
class Job;
}
class KToggleAction;

class QPopupMenu;

namespace KPIM {

class AddresseeView : public KTextBrowser
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
      Returns the HTML representation of a contact.
      The HTML code looks like
        <div>
        <table>
        ...
        </table>
        </div>

      @param addr The addressee object.
      @param useLinks If true, the addresses, emails, phone numbers and urls will
                      be displayed as links.
                      The links looks like this:
                        "addr://<addr id>" for addresses
                        "mailto:<email address>" for emails
                        "phone://<phone number>" for phone numbers
                        "http://<url>" for urls
      @param internalLoading If true, the loading of internal pictures is done automatically.
      @param showBirthday If true, the birthday is included.
      @param showAddresses If true, the addresses are included.
      @param showEmails If true, the emails are included.
      @param showPhones If true, the phone numbers are included.
      @param showURLs If true, the urls are included.
     */
    static QString vCardAsHTML( const KABC::Addressee& addr, bool useLinks = true,
                                bool internalLoading = true,
                                bool showBirthday = true, bool showAddresses = true,
                                bool showEmails = true, bool showPhones = true,
                                bool showURLs = true );
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
    virtual void faxNumberClicked( const QString &number );

    virtual QPopupMenu *createPopupMenu( const QPoint& );

  private slots:
    void slotMailClicked( const QString&, const QString& );
    void slotUrlClicked( const QString& );
    void slotHighlighted( const QString& );
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

    KABC::Addressee mAddressee;

    class AddresseeViewPrivate;
    AddresseeViewPrivate *d;
};

}

#endif
