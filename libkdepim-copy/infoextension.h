/*
   This file is part of libkdepim.

   Copyright (C) 2003 Sven Lüppken <sven@kde.org>

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

#ifndef  INFOEXTENSION_H
#define  INFOEXTENSION_H

#include <qobject.h>
#include <kdepimmacros.h>

class QWidget;

namespace KParts
{

	class ReadOnlyPart;

  /**
   * Provides a way to get information out of a PIM-Part
   **/
	class KDE_EXPORT InfoExtension : public QObject
	{
		Q_OBJECT

	public:
      /**
       * Constucts an InfoExtension.
       *
       * @param parent   The parent widget.
       * @param name     The name of the class.
       **/
		InfoExtension( KParts::ReadOnlyPart *parent, const char* name);
		~InfoExtension();

	private:
		class InfoExtensionPrivate;
		InfoExtensionPrivate *d;

	signals:
		void textChanged( const QString& );
		void iconChanged( const QPixmap& );
  	};
}
#endif // INFOEXTENSION_H
