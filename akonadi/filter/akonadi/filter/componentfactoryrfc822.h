/****************************************************************************** *
 *
 *  File : componentfactoryrfc822.h
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

#ifndef _AKONADI_FILTER_COMPONENTFACTORYRFC822_H_
#define _AKONADI_FILTER_COMPONENTFACTORYRFC822_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/componentfactory.h>

namespace Akonadi
{
namespace Filter
{

/**
 * The feature bits this kind of filter uses
 */
enum FeaturesRfc822
{
  /**
   * This object contains e-mail addresses with comments.
   *
   * This is used on the message header data members
   * that are known to contain address lists.
   */
  FeatureRfc822ContainsAddressesWithComments = FeatureCustomFirstBit,

  /**
   * This object contains e-mail addresses.
   *
   * This is used on the functions that extract e-mail
   * addresses from fields that are known to contain
   * addresses with comments.
   */
  FeatureRfc822ContainsAddresses = FeatureCustomFirstBit << 1
};

/**
 * The data members supported by the ComponentFactoryRfc822 class.
 */
enum DataMemberRfc822Identifiers
{
  DataMemberRfc822FromHeader = DataMemberCustomFirst,
  DataMemberRfc822ToHeader,
  DataMemberRfc822ReplyToHeader,
  DataMemberRfc822SubjectHeader,
  DataMemberRfc822CcHeader,
  DataMemberRfc822BccHeader,
  DataMemberRfc822MessageID,
  DataMemberRfc822Date,
  DataMemberRfc822ResentDate,
  DataMemberRfc822Organization,
  DataMemberRfc822References,
  DataMemberRfc822UserAgent,
  DataMemberRfc822InReplyTo,
  DataMemberRfc822AllRecipientHeaders,
  DataMemberRfc822AllHeaders,
  DataMemberRfc822MessageBody,
  DataMemberRfc822WholeMessage,
  DataMemberRfc822HasAttachments,
  DataMemberRfc822EncodedAttachmentList

}; // enum DataMemberIdentifiersRfc822

/**
 * The commands supported by the ComponentFactoryRfc822 class.
 */
enum CommandRfc822Identifiers
{
  /**
   * The standard "Move Message to Collection" command.
   * This is Akonadi based, non terminal and accepts an integer argument
   * which is the id of the target Akonadi::Collection.
   * In sieve this is encoded as "movetocollection <id>".
   */
  CommandRfc822MoveMessageToCollection = CommandCustomFirst,

  /**
   * The standard "Copy Message to Collection" command.
   * This is Akonadi based, non terminal and accepts an integer argument
   * which is the id of the target Akonadi::Collection.
   * In sieve this is encoded as "movetocollection <id>".
   */
  CommandRfc822CopyMessageToCollection,

  /**
   * The "Delete Message" command.
   * This is Akonadi based, terminal and has no arguments.
   */
  CommandRfc822DeleteMessage,

  /**
   * The "Run program" command.
   * Non terminal with one argument.
   */
  CommandRfc822RunProgram,

  /**
   * The "Pipe through" command.
   * Non terminal with one argument.
   */
  CommandRfc822PipeThrough,

  /**
   * The "Play sound" command.
   * Non terminal with one argument.
   */
  CommandRfc822PlaySound

}; // enum CommandRfc822Identifiers

enum FunctionRfc822Identifiers
{
  FunctionRfc822AnyEMailAddressIn = FunctionCustomFirst,
  FunctionRfc822AnyEMailAddressDomainIn,
  FunctionRfc822AnyEMailAddressLocalPartIn

}; // enum FunctionRfc822Identifiers

enum OperatorRfc822Identifiers
{
  OperatorRfc822IsInAddressbook = OperatorCustomFirst

}; // enum OperatorRfc822Identifiers

/**
 * This class is a specialized implementation of ComponentFactory
 * aimed at filtering Akonadi based message/rfc822 PIM items.
 *
 * In graphical interfaces you should use the EditorFactoryRfc822 
 * subclass which provides editor for the commands that this class provides.
 */
class AKONADI_FILTER_EXPORT ComponentFactoryRfc822 : public ComponentFactory
{
public:
  ComponentFactoryRfc822();
  virtual ~ComponentFactoryRfc822();

}; // class ComponentFactoryRfc822

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMPONENTFACTORYRFC822_H_
