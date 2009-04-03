Resource.setType( "akonadi_maildir_resource" );

// read test
Resource.setPathOption( "Path", "maildir/root" );
Resource.create();

XmlOperations.setXmlFile( "maildir.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.setNormalizeRemoteIds( true );
XmlOperations.ignoreCollectionField( "Name" );
XmlOperations.assertEqual();

Resource.destroy();

// empty maildir
Resource.setPathOption( "Path", "newmaildir" );
Resource.create();

XmlOperations.setXmlFile( "maildir-empty.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();

// folder creation
CollectionTest.setParent( Resource.identifier() );
CollectionTest.addContentType( "message/rfc822" );
CollectionTest.setName( "test folder" );
CollectionTest.create();

alert( "wait" );

// item creation
ItemTest.setParentCollection( Resource.identifier() + "/test folder" );
ItemTest.setMimeType( "message/rfc822" );
ItemTest.setPayloadFromFile( "testmail.mbox" );
ItemTest.create();

Resource.recreate();

XmlOperations.setXmlFile( "maildir-step1.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.setItemKey( "None" );
XmlOperations.ignoreItemField( "RemoteId" );
XmlOperations.assertEqual();

// folder modification (doesn't work yet)
/*CollectionTest.setCollection( Resource.identifier() + "/test folder" );
CollectionTest.setName( "changed folder" );
CollectionTest.update();

Resource.recreate();

XmlOperations.setXmlFile( "maildir-step2.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();*/

// folder deletion (not implemented yet either)
/*CollectionTest.setCollection( Resource.identifier() + "/changed folder" );
CollectionTest.setCollection( Resource.identifier() + "/test folder" );
CollectionTest.remove();

Resource.recreate();

XmlOperations.setXmlFile( "maildir-empty.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();*/

