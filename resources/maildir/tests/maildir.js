Resource.setType( "akonadi_maildir_resource" );
Resource.setPathOption( "Path", "maildir/root" );
Resource.create();

XmlOperations.setXmlFile( "maildir.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();

