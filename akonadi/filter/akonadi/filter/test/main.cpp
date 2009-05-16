
#include "../filter.h"
#include "../sievedecoder.h"
#include "../program.h"
#include "../componentfactory.h"
#include "../condition.h"

#include <QTextStream>

class MyComponentFactory : public Akonadi::Filter::ComponentFactory
{
public:
  MyComponentFactory(){};
  virtual ~MyComponentFactory(){};
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
  Akonadi::Filter::IO::SieveDecoder d( new MyComponentFactory() );
  Akonadi::Filter::Program * p = d.run();
  if( p )
    delete p;


  return 0;
}
