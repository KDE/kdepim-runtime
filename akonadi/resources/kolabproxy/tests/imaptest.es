/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

/** helper function to run IMAP commands for the initial VM setup */
runImapCmd = function( user, cmd )
{
  var args = ["--port", QEmu.portOffset() + 143, "--user", user + "@example.com", "--password", "nichtgeheim" ];
  args = args.concat( cmd );
  System.exec( "runimapcommand.py", args );
}

/** creates two user accounts with a standard set of Kolab folders shared between two users */
setupServer = function()
{
  System.exec( "create_ldap_users.py", ["-h", "localhost", "-p", QEmu.portOffset() + 389,
    "-n", "2", "--set-password", "nichtgeheim", "dc=example,dc=com", "add", "nichtgeheim" ] );

  // Wait until the user accounts have been fully created
  runImapCmd( "autotest0", [ "waitformailbox", "INBOX"] );
  runImapCmd( "autotest1", [ "waitformailbox", "INBOX"] );

  // creates folders and messages for the primary test user
  runImapCmd( "autotest0", [ "create", "INBOX/Calendar" ] );
  runImapCmd( "autotest0", [ "append", "INBOX", "testmail.mbox" ] );

  // creates folders and messages for the secondary test user and share them with the first one
  runImapCmd( "autotest1", [ "create", "INBOX/child1" ] );
  runImapCmd( "autotest1", [ "create", "INBOX/child2" ] );
  runImapCmd( "autotest1", [ "create", "INBOX/child1/grandchild1" ] );
  runImapCmd( "autotest1", [ "create", "INBOX/child2/grandchild1" ] );
  runImapCmd( "autotest1", [ "append", "INBOX/child1/grandchild1", "testmail.mbox" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/child1", "autotest0@example.com", "lrs" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/child2", "autotest0@example.com", "lrs" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/child1/grandchild1", "autotest0@example.com", "lrs" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/child2/grandchild1", "autotest0@example.com", "lrs" ] );
}

/** The actual tests for the IMAP resource, @p vm is the name of the server VM config file. */
function testImap( vm )
{
  QEmu.setVMConfig( vm + "vm.conf" );
  QEmu.start();

  setupServer();

  var imapResource = Resource.newInstance( "akonadi_imap_resource" );
  imapResource.setOption( "ImapServer", "localhost:42143" );
  imapResource.setOption( "UserName", "autotest0@example.com" );
  imapResource.setOption( "Password", "nichtgeheim" );
  imapResource.create();

  // verify reading by checking if we can read the initial server state
  XmlOperations.setXmlFile( "imap-step1.xml" );
  XmlOperations.setRootCollections( imapResource.identifier() );
  // FIXME: one of the attributes contains a current date/time breaking the comparison
  XmlOperations.ignoreCollectionField( "Attributes" );
  XmlOperations.assertEqual();

  // folder creation
  CollectionTest.setParent( "localhost:42143\\/autotest0@example.com/INBOX" );
  CollectionTest.addContentType( "message/rfc822" );
  CollectionTest.setName( "test folder" );
  CollectionTest.create();

  // item creation
  ItemTest.setParentCollection( "localhost:42143\\/autotest0@example.com/INBOX/test folder" );
  ItemTest.setMimeType( "message/rfc822" );
  ItemTest.setPayloadFromFile( "testmail.mbox" );
  ItemTest.create();

  imapResource.recreate();
  XmlOperations.setXmlFile( "imap-step2.xml" );
  XmlOperations.setRootCollections( imapResource.identifier() );
  // FIXME: one of the attributes contains a current date/time breaking the comparison
  XmlOperations.ignoreCollectionField( "Attributes" );
  XmlOperations.assertEqual();

  // TODO: test change, move, item delete

  // folder deletion
  CollectionTest.setCollection( "localhost:42143\\/autotest0@example.com/INBOX/test folder" );
  CollectionTest.remove();

  XmlOperations.setXmlFile( "imap-step1.xml" );
  XmlOperations.setRootCollections( imapResource.identifier() );
  // FIXME: one of the attributes contains a current date/time breaking the comparison
  XmlOperations.ignoreCollectionField( "Attributes" );
  XmlOperations.assertEqual();

  imapResource.destroy();
  QEmu.stop();
}
