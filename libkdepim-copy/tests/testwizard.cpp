/*
    This file is part of libkdepim.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "myconfig.h"

#include <kconfigwizard.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include <qlayout.h>
#include <qcheckbox.h>

class TestConfigWizard : public KConfigWizard
{
  public:
    TestConfigWizard() :
      KConfigWizard( new KConfigPropagator( MyConfig::self(),
                                            "propagator_test.kcfg" ) )
    {
      QFrame *page = createWizardPage( "My Wizard Page" );
      QBoxLayout *topLayout = new QVBoxLayout( page );
      
      mFixKMailCheckBox = new QCheckBox( i18n("Fix KMail"), page );      
      topLayout->addWidget( mFixKMailCheckBox );

      mFixKMailCheckBox->setChecked( MyConfig::fixKMail() );
    
      mBreakKMailCheckBox = new QCheckBox( i18n("Break KMail"), page );      
      topLayout->addWidget( mBreakKMailCheckBox );

      mBreakKMailCheckBox->setChecked( MyConfig::breakKMail() );
    
      setupRulesPage();
      setupChangesPage();
    }
    
    ~TestConfigWizard()
    {
    }

    void usrReadConfig()
    {
    }

    void usrWriteConfig()
    {
      MyConfig::self()->setFixKMail( mFixKMailCheckBox->isChecked() );
      MyConfig::self()->setBreakKMail( mBreakKMailCheckBox->isChecked() );
    }

  private:
    QCheckBox *mFixKMailCheckBox;
    QCheckBox *mBreakKMailCheckBox;
};

static const KCmdLineOptions options[] =
{
  {"verbose", "Verbose output", 0},
  KCmdLineLastOption
};

int main(int argc,char **argv)
{
  KAboutData aboutData("testwizard","Test KConfigWizard","0.1");
  KCmdLineArgs::init(argc,argv,&aboutData);
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  bool verbose = false;
  if ( args->isSet( "verbose" ) ) verbose = true;

  TestConfigWizard wizard;
  
  wizard.exec();
}
