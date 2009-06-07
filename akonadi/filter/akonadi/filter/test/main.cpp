
#include <akonadi/filter/sievedecoder.h>
#include <akonadi/filter/program.h>
#include <akonadi/filter/componentfactory.h>
#include <akonadi/filter/condition.h>

#include <akonadi/filter/ui/programeditor.h>
#include <akonadi/filter/ui/editorfactory.h>


#include <KCmdLineArgs>
#include <KApplication>
#include <KAboutData>
#include <KDebug>

class MyComponentFactory : public Akonadi::Filter::ComponentFactory
{
public:
  MyComponentFactory() : ComponentFactory() {};
  virtual ~MyComponentFactory(){};
public:
  virtual Akonadi::Filter::Action::Base * createGenericAction( Akonadi::Filter::Component * parent, const QString &identifier )
  {
    return new Akonadi::Filter::Action::Stop( parent );
  }
  virtual Akonadi::Filter::Condition::Base * createGenericCondition( Akonadi::Filter::Component * parent, const QString &identifier )
  {
    return new Akonadi::Filter::Condition::True( parent );
  }
};


int main(int argc,char ** argv)
{

  MyComponentFactory * f = new MyComponentFactory();
  f->setDefaultActionDescription( i18n( "download message" ) );
  Akonadi::Filter::IO::SieveDecoder d( f );
  Akonadi::Filter::Program * p = d.run();
  if( p )
  {
    const QByteArray& ba = QByteArray( "test" );
    const KLocalizedString name = ki18n( "Test" );
    KAboutData aboutData( ba, ba, name, ba, name );
    KCmdLineArgs::init( argc, argv, &aboutData );
    KApplication app;

    Akonadi::Filter::UI::EditorFactory * ef = new Akonadi::Filter::UI::EditorFactory();

    Akonadi::Filter::UI::ProgramEditor * ed = new Akonadi::Filter::UI::ProgramEditor( 0, f, ef );
    ed->fillFromProgram( p );
    delete p;

    ed->show();
    
    app.exec();

    p = ed->commit();
    if( p )
    {
      qDebug( "Edited program dump" );
      p->dump( QString() );
      delete p;
    } else {
      qDebug( "Could not commit program" );
    }

    delete ed;

    delete ef;
  }

  delete f;

  return 0;
}
