
#include "../filter.h"


int main(int argc,char ** argv)
{
  Akonadi::Filter f;

  if ( argc < 2 )
  {
    qDebug( "You must pass a sieve script file name" );
    return -1;
  }

  if ( f.loadSieveScript( QString::fromAscii( argv[1] ) ) )
    qDebug( "Sieve script loaded" );
  else
    qDebug( "Could not load sieve script" );

  return 0;
}
