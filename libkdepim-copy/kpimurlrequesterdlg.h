/* This file is part of the KDE libraries
    Copyright (C) 2000 Wilco Greven <greven@kde.org>

    library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KPIMURLREQUESTDLG_H_INCLUDED
#define KPIMURLREQUESTDLG_H_INCLUDED

#include <kdepimmacros.h>
#include <kurl.h>
#include <qstring.h>

class QWidget;

class KDE_EXPORT KPimURLRequesterDlg
{
	public:
    /**
     * Creates a modal dialog with the given label text, executes it and 
		 * returns the selected URL.
     *
     * @param url This specifies the initial path of the input line.
		 * @param text The text to be shown on the label.
     * @param parent The widget the dialog will be centered on initially.
     */
		static KURL getURL( const QString &url = QString::null, 
			                const QString &text = QString::null,
                      QWidget *parent = 0, 
			                const QString &caption = QString::null );
};

#endif // KPIMURLREQUESTDLG_H_INCLUDED

