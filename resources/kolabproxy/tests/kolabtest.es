function runImapCmd( cmd )
{
  var args = ["--port", QEmu.portOffset() + 143, "--user", "autotest0@example.com", "--password", "nichtgeheim" ];
  args = args.concat( cmd );
  System.exec( "runimapcommand.py", args );
}

QEmu.setVMConfig( "kolabvm.conf" );
QEmu.start();

System.exec( "create_ldap_users.py", ["-h", "localhost", "-p", QEmu.portOffset() + 389,
  "-n", "1", "--set-password", "nichtgeheim", "dc=example,dc=com", "add", "nichtgeheim" ] );

// the server needs some time to update the user db apparently
System.sleep( 5 );

runImapCmd( [ "create", "INBOX/Calendar" ] );
runImapCmd( [ "setannotation", "INBOX/Calendar", "/vendor/kolab/folder-type", "event.default" ] );
runImapCmd( [ "append", "INBOX", "testmail.mbox" ] );

var imapResource = Resource.newInstance();
imapResource.setType( "akonadi_imap_resource" );
imapResource.setOption( "ImapServer", "localhost:42143" );
imapResource.setOption( "UserName", "autotest0@example.com" );
imapResource.create();

Test.alert( "wait" );

