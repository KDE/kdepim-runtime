function runImapCmd( cmd )
{
  var args = ["--port", QEmu.portOffset() + 143, "--user", "autotest0@example.com", "--password", "nichtgeheim" ];
  args = args.concat( cmd );
  System.exec( "runimapcommand.py", args );
}

function testImap( vm ) {
  QEmu.setVMConfig( vm );
  QEmu.start();

  System.exec( "create_ldap_users.py", ["-h", "localhost", "-p", QEmu.portOffset() + 389,
    "-n", "1", "--set-password", "nichtgeheim", "dc=example,dc=com", "add", "nichtgeheim" ] );

  // the server needs some time to update the user db apparently
  System.sleep( 20 );

  runImapCmd( [ "create", "INBOX/Calendar" ] );
  runImapCmd( [ "append", "INBOX", "testmail.mbox" ] );

  var imapResource = Resource.newInstance( "akonadi_imap_resource" );
  imapResource.setOption( "ImapServer", "localhost:42143" );
  imapResource.setOption( "UserName", "autotest0@example.com" );
  imapResource.setOption( "Password", "nichtgeheim" );
  imapResource.create();

  XmlOperations.setXmlFile( "imap-step1.xml" );
  XmlOperations.setRootCollections( imapResource.identifier() );
  // FIXME: one of the attributes contains a current date/time breaking the comparison
  XmlOperations.ignoreCollectionField( "Attributes" );
  XmlOperations.assertEqual();

  // TODO: test changing stuff

  imapResource.destroy();
  QEmu.stop();
}

testImap( "kolabvm.conf" );
testImap( "dovecotvm.conf" );
