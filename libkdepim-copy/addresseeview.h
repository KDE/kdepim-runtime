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

#include <kabc/addressee.h>
#include <ktextbrowser.h>

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

  signals:
    void urlHighlighted( const QString &url );
    void emailHighlighted( const QString &email );
    void phoneNumberHighlighted( const QString &number );
    void faxNumberHighlighted( const QString &number );

    void highlightedMessage( const QString &message );

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

  private:
    void load();
    void save();

    void updateView();

    QString strippedNumber( const QString &number );

    KConfig *mConfig;
    bool mDefaultConfig;

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
