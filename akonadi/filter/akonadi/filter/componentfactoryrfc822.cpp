/****************************************************************************** *
 *
 *  File : componentfactoryrfc822.cpp
 *  Created on Sun 02 Aug 2009 20:19:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "componentfactoryrfc822.h"

#include <KLocale>
#include <KDebug>

namespace Akonadi
{
namespace Filter 
{


ComponentFactoryRfc822::ComponentFactoryRfc822()
  : ComponentFactory()
{
  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822SubjectHeader,
          QString::fromAscii( "subject" ),
          i18n( "Subject" ),
          DataTypeString
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822FromHeader,
          QString::fromAscii( "from" ),
          i18n( "From address" ),
          DataTypeAddress
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822ToHeader,
          QString::fromAscii( "to" ),
          i18n( "To address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822ReplyToHeader,
          QString::fromAscii( "replyto" ),
          i18n( "Reply-To address" ),
          DataTypeAddress
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822CcHeader,
          QString::fromAscii( "cc" ),
          i18n( "Cc address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822BccHeader,
          QString::fromAscii( "bcc" ),
          i18n( "Bcc address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822AllRecipientHeaders,
          QString::fromAscii( "anyrecipient" ),
          i18n( "recipient address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822AllHeaders,
          QString::fromAscii( "anyheader" ),
          i18n( "header list" ),
          DataTypeStringList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822MessageBody,
          QString::fromAscii( "body" ),
          i18n( "message body" ),
          DataTypeString
        )
    ); 

  registerDataMember(
      new DataMemberDescriptor(
          DataMemberRfc822WholeMessage,
          QString::fromAscii( "message" ),
          i18n( "whole message" ),
          DataTypeString
        )
    );  









  registerFunction(
      new FunctionDescriptor(
          FunctionRfc822AnyEMailAddressIn,
          QString::fromAscii( "address" ),
          i18n( "if any address in" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeAddress | DataTypeAddressList
        )
    );

  registerFunction(
      new FunctionDescriptor(
          FunctionRfc822AnyEMailAddressDomainIn,
          QString::fromAscii( "address:domain" ),
          i18n( "if any domain in" ),
          DataTypeStringList,
          DataTypeAddress | DataTypeAddressList
        )
    );


  registerFunction(
      new FunctionDescriptor(
          FunctionRfc822AnyEMailAddressLocalPartIn,
          QString::fromAscii( "address:local" ),
          i18n( "if any username in" ),
          DataTypeStringList,
          DataTypeAddress | DataTypeAddressList
        )
    );



#if 0
  registerOperator(
      new OperatorDescriptor(
          OperatorRfc822IsInAddressbook,
          QString::fromAscii( "inaddressbook" ),
          i18n( "is in addressbook" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeNone
        )
    );
#endif



  CommandDescriptor * cmd;

#if 0
  cmd = new CommandDescriptor(
      StandardCommandLeaveMessageOnServer,
      QString::fromAscii( "leaveonserver" ),
      i18n( "leave the message on server and stop processing here" ),
      true
    );

  registerCommand( cmd );

  cmd = new CommandDescriptor(
      StandardCommandDownloadMessageButKeepCopyOnServer,
      QString::fromAscii( "downloadbutkeep" ),
      i18n( "download message but keep a copy on server (and stop processing here)" ),
      true
    );

  registerCommand( cmd );

  cmd = new CommandDescriptor(
      StandardCommandDownloadMessageAndDeleteCopyOnServer,
      QString::fromAscii( "downloadanddelete" ),
      i18n( "download message and delete the copy on server (and stop processing here)" ),
      true
    );

  registerCommand( cmd );


  cmd = new CommandDescriptor(
      StandardCommandLeaveMessageOnServer,
      QString::fromAscii( "deleteonserver" ),
      i18n( "delete message on server (and stop processing here)" ),
      true
    );

  registerCommand( cmd );

#endif

  cmd = new CommandDescriptor(
      CommandRfc822DeleteMessage,
      QString::fromAscii( "delete" ),
      i18n( "delete the message (and stop processing here)" ),
      true
    );
  registerCommand( cmd );

  cmd = new CommandDescriptor(
      CommandRfc822MoveMessageToCollection,
      QString::fromAscii( "movetocollection" ),
      i18n( "move the message to folder" ),
      false
    );
  cmd->addParameter( new CommandDescriptor::ParameterDescriptor( DataTypeInteger, i18n( "Target collection id" ) ) );
  registerCommand( cmd );

  cmd = new CommandDescriptor(
      CommandRfc822CopyMessageToCollection,
      QString::fromAscii( "copytocollection" ),
      i18n( "create a copy in folder" ),
      false
    );
  cmd->addParameter( new CommandDescriptor::ParameterDescriptor( DataTypeInteger, i18n( "Target collection id" ) ) );
  registerCommand( cmd );

}

ComponentFactoryRfc822::~ComponentFactoryRfc822()
{
}

} // namespace Filter

} // namespace Akonadi

