/*
    Copyright (c) 2010 Laurent Montel <montel@kde.org>

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

#include "identitypage.h"
#include "identity.h"

IdentityPage::IdentityPage( KAssistantDialog *parent )
  : Page( parent )
{
  ui.setupUi( this );
  connect( ui.mCreateNewIdentity, SIGNAL( clicked ( bool ) ), ui.mEmail, SLOT( setEnabled( bool ) ) );
  connect( ui.mCreateNewIdentity, SIGNAL( clicked ( bool ) ), ui.mRealName, SLOT( setEnabled( bool ) ) );
  connect( ui.mCreateNewIdentity, SIGNAL( clicked ( bool ) ), ui.mOrganization, SLOT( setEnabled( bool ) ) );
  ui.mCreateNewIdentity->setChecked( false );
  setValid(true);
}

IdentityPage::~IdentityPage()
{
}

void IdentityPage::leavePageNext()
{
  if ( ui.mCreateNewIdentity->isChecked() ) {
    Identity *id = new Identity( this );
    id->setRealName( ui.mRealName->text() );
    id->setEmail( ui.mEmail->text() );
    id->setOrganization( ui.mOrganization->text() );
    id->create();
  }
}



#include "identitypage.moc"
