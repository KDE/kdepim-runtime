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

namespace KPIM {

class AddresseeView : public KTextBrowser
{
  Q_OBJECT

  public:
    AddresseeView( QWidget *parent = 0, const char *name = 0 );

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
    void phoneNumberClicked( const QString &number );
    void faxNumberClicked( const QString &number );

  private slots:
    void mailClicked( const QString&, const QString& );
    void urlClicked( const QString& );

  private:
    KABC::Addressee mAddressee;
    
    class AddresseeViewPrivate;
    AddresseeViewPrivate *d;
};

}

#endif
