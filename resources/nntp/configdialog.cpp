/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "configdialog.h"
#include "settings.h"

#include <kconfigdialogmanager.h>
#include <KLocalizedString>
#include <QVBoxLayout>
#include <QDialogButtonBox>

ConfigDialog::ConfigDialog(QWidget * parent) :
    QDialog( parent )
{
  QWidget *mainWidget = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->addWidget(mainWidget);
  ui.setupUi(mainWidget);
  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();
  ui.password->setText( Settings::self()->password() );
  ui.kcfg_MaxDownload->setSuffix( ki18np( " article", " articles" ) );

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  mainLayout->addWidget(buttonBox);

  connect(okButton, SIGNAL(clicked()), SLOT(save()) );
}

void ConfigDialog::save()
{
  if ( ui.kcfg_StorePassword->isChecked() )
    Settings::self()->setPassword( ui.password->text() );
  mManager->updateSettings();
}

