/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>
  
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nepomukfeederagentdialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <KIntNumInput>
#include <KLocale>
#include <KConfigGroup>

NepomukFeederAgentDialog::NepomukFeederAgentDialog(QWidget *parent)
  :KDialog(parent)
{
  setCaption( i18n( "Configure Nepomuk Feeder Agent" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  QWidget *mainWidget = new QWidget( this );
  QVBoxLayout *mainLayout = new QVBoxLayout( mainWidget );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( KDialog::marginHint() );

  mDisableIdleTimeOut = new QCheckBox(i18n("Disable Idle Timeout"));
  mainLayout->addWidget(mDisableIdleTimeOut);

  QHBoxLayout *lay = new QHBoxLayout;
  QLabel *lab = new QLabel(i18n("Idle TimeOut"));
  lay->addWidget(lab);

  mTimeOut = new KIntNumInput;
  mTimeOut->setMinimum(120);
  mTimeOut->setSuffix(i18n(" ms"));
  lay->addWidget(mTimeOut);

  mainLayout->addLayout(lay);

  setMainWidget( mainWidget );
  connect(this,SIGNAL(okClicked()),SLOT(slotSave()));
  readConfig();
}

NepomukFeederAgentDialog::~NepomukFeederAgentDialog()
{

}

void NepomukFeederAgentDialog::slotSave()
{
  KConfig config(QLatin1String("akonadi_nepomuk_feederrc"));
  KConfigGroup grp = config.group(QLatin1String("akonadi_nepomuk_feeder"));
  grp.writeEntry( "IdleTimeout", mTimeOut->value() );
  grp.readEntry( "DisableIdleDetection", mDisableIdleTimeOut->isChecked() );
  grp.sync();
}

void NepomukFeederAgentDialog::readConfig()
{
  KConfig config(QLatin1String("akonadi_nepomuk_feederrc"));
  KConfigGroup grp = config.group(QLatin1String("akonadi_nepomuk_feeder"));
  mTimeOut->setValue(grp.readEntry( "IdleTimeout", 120 ));
  mDisableIdleTimeOut->setChecked(grp.readEntry( "DisableIdleDetection", false ));
}
