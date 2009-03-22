Resource.setType( "akonadi_vcard_resource" );
Resource.setPathOption( "Path", "vcardtest.vcf" );
Resource.create();

alert( "test" );

XmlOperations.setXmlFile( "vcardtest.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();

