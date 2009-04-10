var i1 = ItemTest.newInstance();
var i2 = ItemTest.newInstance();

print( i1 );
print( i2 );

i1.setMimeType( "application/foo" );
i2.setMimeType( "application/bar" );

print( i1.mimeType() + " != " + i2.mimeType() );
Test.assert( i1.mimeType() != i2.mimeType() );

