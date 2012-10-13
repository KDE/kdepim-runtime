/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>
  Copyright (c) 2012 Allen Winter <winter@kde.org>

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
#include <QDebug>
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
  mDisableIdleTimeOut->setEnabled( true );
  mDisableIdleTimeOut->setChecked( true );
  mainLayout->addWidget(mDisableIdleTimeOut);

  QHBoxLayout *lay = new QHBoxLayout;
  QLabel *lab = new QLabel(i18n("Idle Timeout"));
  lay->addWidget(lab);

  mTimeOut = new KIntNumInput;
  mTimeOut->setMinimum(120);
  mTimeOut->setSuffix(i18nc("define timeout in seconds", " s"));
  mTimeOut->setEnabled( false );
  lay->addWidget(mTimeOut);

  mainLayout->addLayout(lay);

  setMainWidget( mainWidget );
  connect( mDisableIdleTimeOut, SIGNAL(toggled(bool)), SLOT(slotTimeoutToggle()) );
  connect( this, SIGNAL(okClicked()), SLOT(slotSave()) );
  readConfig();
}

NepomukFeederAgentDialog::~NepomukFeederAgentDialog()
{

}

void NepomukFeederAgentDialog::slotSave()
{
  KConfigGroup grp( KGlobal::config(), QLatin1String("akonadi_nepomuk_feeder") );
  grp.writeEntry( "IdleTimeout", mTimeOut->value() );
  grp.writeEntry( "DisableIdleDetection", mDisableIdleTimeOut->isChecked() );
  grp.sync();
}

void NepomukFeederAgentDialog::readConfig()
{
  KConfigGroup grp( KGlobal::config(), QLatin1String("akonadi_nepomuk_feeder") );
  mDisableIdleTimeOut->setChecked(grp.readEntry( "DisableIdleDetection", true ));
  mTimeOut->setValue(grp.readEntry( "IdleTimeout", 120 ));
  mTimeOut->setEnabled( !mDisableIdleTimeOut->isChecked() );
}

void NepomukFeederAgentDialog::slotTimeoutToggle()
{
  mTimeOut->setEnabled( !mDisableIdleTimeOut->isChecked() );

}

#include "nepomukfeederagentdialog.moc"
