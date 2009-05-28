/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef MESSAGEQUEUER_H
#define MESSAGEQUEUER_H

#include <KVBox>
#include <mailtransport/transportcombobox.h>

class KJob;
class KLineEdit;
class KTextEdit;


/**
  This is stolen from kdepimlibs/mailtransport/tests/transportmgr.{h,cpp}
*/
class MessageQueuer : public KVBox
{
  Q_OBJECT

  public:
    MessageQueuer();

  private slots:
    void sendBtnClicked();
    void jobResult( KJob *job );
    void jobPercent( KJob *job, unsigned long percent );
    void jobInfoMessage( KJob *job, const QString &info, const QString &info2 );

  private:
    MailTransport::TransportComboBox *mComboBox;
    KLineEdit *mSenderEdit, *mToEdit, *mCcEdit, *mBccEdit;
    KTextEdit *mMailEdit;

};


#endif
