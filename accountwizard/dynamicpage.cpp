/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "dynamicpage.h"

#include <KDebug>

#include <QUiLoader>
#include <QFile>
#include <qboxlayout.h>

DynamicPage::DynamicPage(const QString& uiFile, KAssistantDialog* parent) : Page( parent )
{
  QUiLoader loader;
  QFile file( uiFile );
  file.open( QFile::ReadOnly );
  kDebug() << uiFile;
  QWidget *dynamicWidget = loader.load( &file, this );
  file.close();

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget( dynamicWidget );
  setLayout( layout );

  setValid( true );
}
