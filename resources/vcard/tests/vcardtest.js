Resource.setType( "akonadi_vcard_resource" );
Resource.setPathOption( "Path", "vcardtest.vcf" );
Resource.create();

XmlOperations.setXmlFile( "vcardtest.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.ignoreCollectionField( "Name" ); // name is the resource identifier and thus unpredictable
XmlOperations.assertEqual();

