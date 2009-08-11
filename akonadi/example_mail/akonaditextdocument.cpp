/*
    This file is part of the Akonadi Mail example.

    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include "akonaditextdocument.h"

#include <akonadi/entitytreemodel.h>
#include <QImage>

#include <KUrl>
#include <akonaditemplateloader.h>

using namespace Akonadi;

TextDocument::TextDocument(AkonadiTemplateLoader *loader, QObject* parent)
    : QTextDocument(parent), m_templateLoader(loader)
{

}

QVariant TextDocument::loadResource(int type, const QUrl& name)
{
  if (type != QTextDocument::ImageResource)
    return QTextDocument::loadResource(type, name);

  Item requestedItem = m_templateLoader->getItem( KUrl( name ) );

  if (!requestedItem.isValid())
    return QTextDocument::loadResource(type, name);

  if (!requestedItem.hasPayload<QImage>())
    return QTextDocument::loadResource(type, name);

  QImage image = requestedItem.payload<QImage>();

  if (!image.isNull())
    return image;

  return QTextDocument::loadResource(type, name);
}

