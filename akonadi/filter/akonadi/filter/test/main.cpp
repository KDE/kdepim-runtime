
#include <akonadi/filter/sievedecoder.h>
#include <akonadi/filter/program.h>
#include <akonadi/filter/factory.h>
#include <akonadi/filter/condition.h>

#include <akonadi/filter/ui/programeditor.h>


#include <KCmdLineArgs>
#include <KApplication>
#include <KAboutData>

class MyFactory : public Akonadi::Filter::Factory
{
public:
  MyFactory() : Factory() {};
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

  MyFactory * f = new MyFactory();
  Akonadi::Filter::IO::SieveDecoder d( f );
  Akonadi::Filter::Program * p = d.run();
  if( p )
  {
    const QByteArray& ba = QByteArray( "test" );
    const KLocalizedString name = ki18n( "Test" );
    KAboutData aboutData( ba, ba, name, ba, name );
    KCmdLineArgs::init( argc, argv, &aboutData );
    KApplication app;


    Akonadi::Filter::UI::ProgramEditor * ed = new Akonadi::Filter::UI::ProgramEditor( 0, f );
    ed->fillFromProgram( p );

    ed->show();
    
    app.exec();

    delete ed;

    delete p;
  }

  delete f;

  return 0;
}
