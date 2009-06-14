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
function runImapCmd( user, cmd )
{
  var args = ["--port", QEmu.portOffset() + 143, "--user", user + "@example.com", "--password", "nichtgeheim" ];
  args = args.concat( cmd );
  System.exec( "runimapcommand.py", args );
}

/** helper function to create a Kolab folder setup */
function createKolabFolders( user )
{
  runImapCmd( user, [ "create", "INBOX/Calendar" ] );
  runImapCmd( user, [ "create", "INBOX/Contacts" ] );
  runImapCmd( user, [ "create", "INBOX/Journal" ] );
  runImapCmd( user, [ "create", "INBOX/Notes" ] );
  runImapCmd( user, [ "create", "INBOX/Tasks" ] );

  runImapCmd( user, [ "setannotation", "INBOX/Calendar", "/vendor/kolab/folder-type", "event.default" ] );
  runImapCmd( user, [ "setannotation", "INBOX/Contacts", "/vendor/kolab/folder-type", "contact.default" ] );
  runImapCmd( user, [ "setannotation", "INBOX/Journal", "/vendor/kolab/folder-type", "journal.default" ] );
  runImapCmd( user, [ "setannotation", "INBOX/Notes", "/vendor/kolab/folder-type", "note.default" ] );
  runImapCmd( user, [ "setannotation", "INBOX/Tasks", "/vendor/kolab/folder-type", "task.default" ] );
}

/** creates two user accounts with a standard set of Kolab folders shared between two users */
function setupKolab()
{
  System.exec( "create_ldap_users.py", ["-h", "localhost", "-p", QEmu.portOffset() + 389,
    "-n", "2", "--set-password", "nichtgeheim", "dc=example,dc=com", "add", "nichtgeheim" ] );

  // the server needs some time to update the user db apparently
  System.sleep( 20 );

  // creates folders and messages for the primary test user
  createKolabFolders( "autotest0" );
  runImapCmd( "autotest0", [ "append", "INBOX/Calendar", "kolabevent.mbox" ] );

  // create folders and messages for the user sharing his folders with the test user
/*  createKolabFolders( "autotest1" );
  runImapCmd( "autotest1", [ "setacl", "INBOX/Calendar", "autotest0@example.com", "lrs" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/Contacts", "autotest0@example.com", "lrs" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/Journal", "autotest0@example.com", "lrs" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/Notes", "autotest0@example.com", "lrs" ] );
  runImapCmd( "autotest1", [ "setacl", "INBOX/Tasks", "autotest0@example.com", "lrs" ] );*/
}

/** The actual tests for the Kolab resource. */
function testKolab( vm )
{
  QEmu.setVMConfig( vm );
  QEmu.start();

  setupKolab();

  var imapResource = Resource.newInstance( "akonadi_imap_resource" );
  imapResource.setOption( "ImapServer", "localhost:42143" );
  imapResource.setOption( "UserName", "autotest0@example.com" );
  imapResource.setOption( "Password", "nichtgeheim" );
  imapResource.create();

  Test.alert( "wait" ); // FIXME

  XmlOperations.setXmlFile( "kolab-step1.xml" );
  XmlOperations.setRootCollections( kolabResource.identifier() );
  XmlOperations.setCollectionKey( "Name" );
  XmlOperations.ignoreCollectionField( "RemoteId" );
  XmlOperations.setItemKey( "None" ); // FIXME this should not be necessary when using the testrunner?
  XmlOperations.ignoreItemField( "RemoteId" );
  XmlOperations.assertEqual();

  // TODO: adding/changing/removing collections, adding/changing/removing items, in both directions

  var eventItem = ItemTest.newInstance();
  eventItem.setMimeType( "application/x-vnd.akonadi.calendar.event" );
  eventItem.setParentCollection( "akonadi_kolabproxy_resource/Calendar" );
  eventItem.setPayloadFromFile( "event.ical" );
  eventItem.create();

  var taskItem = ItemTest.newInstance();
  taskItem.setMimeType( "application/x-vnd.akonadi.calendar.todo" );
  taskItem.setParentCollection( "akonadi_kolabproxy_resource/Tasks" );
  taskItem.setPayloadFromFile( "task.ical" );
  taskItem.create();

  Test.alert( "wait again" ); // FIXME

  // FIXME I need either two instances or a way to reset this thing...
  XmlOperations.setXmlFile( "kolab-step2.xml" );
  XmlOperations.setRootCollections( imapResource.identifier() );
  XmlOperations.setCollectionKey( "RemoteId" );
  // FIXME: one of the attributes contains a current date/time breaking the comparison
  XmlOperations.ignoreCollectionField( "Attributes" );
  XmlOperations.setItemKey( "RemoteId" );
  // FIXME: payload contains a current date/time...
  XmlOperations.ignoreItemField( "Payload" );
  XmlOperations.assertEqual();

  Test.alert( "done" );
  imapResource.destroy();
  QEmu.stop();
}

var kolabResource = Resource.newInstance( "akonadi_kolabproxy_resource" );
kolabResource.create();

testKolab( "kolabvm.conf" );
// testKolab( "dovecotvm.conf" );

kolabResource.destroy();
