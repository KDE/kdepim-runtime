
#include "../filter.h"
#include "../sievedecoder.h"
#include "../program.h"
#include "../componentfactory.h"

#include <QTextStream>

int main(int argc,char ** argv)
{
  Akonadi::Filter::IO::SieveDecoder d( new Akonadi::Filter::ComponentFactory() );
  Akonadi::Filter::Program * p = d.run();
  if( p )
    delete p;


  return 0;
}
