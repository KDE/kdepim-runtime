
#include "../filter.h"
#include "../sievedecoder.h"
#include "../program.h"
#include "../factory.h"
#include "../condition.h"

#include <QTextStream>

class MyFactory : public Akonadi::Filter::Factory
{
public:
  MyFactory(){};
  virtual ~MyFactory(){};
public:
  virtual Akonadi::Filter::Action::Base * createGenericAction( Akonadi::Filter::Component * parent, const QString &identifier )
  {
    return new Akonadi::Filter::Action::Base( parent );
  }
  virtual Akonadi::Filter::Condition::Base * createGenericCondition( Akonadi::Filter::Component * parent, const QString &identifier )
  {
    return new Akonadi::Filter::Condition::Base( Akonadi::Filter::Condition::Base::ConditionTypeAnd, parent );
  }
};


int main(int argc,char ** argv)
{
  Akonadi::Filter::IO::SieveDecoder d( new MyFactory() );
  Akonadi::Filter::Program * p = d.run();
  if( p )
    delete p;


  return 0;
}
