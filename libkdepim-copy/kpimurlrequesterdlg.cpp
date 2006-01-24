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


#include <kpimurlrequesterdlg.h>
#include <krecentdocument.h>
#include <kurlrequesterdlg.h>
#include <klocale.h>

// KDE3.4 or KDE4.0: FIXME: Move to kdelibs!
KUrl KPimURLRequesterDlg::getURL( const QString& dir, const QString &label,
                                  QWidget *parent, const QString& caption )
{
    KUrlRequesterDlg dlg(dir, label, parent, "filedialog", true);

    dlg.setCaption(caption.isNull() ? i18n("Open") : caption);

    dlg.exec();

    const KUrl& url = dlg.selectedURL();
    if (url.isValid())
        KRecentDocument::add(url);

    return url;
}
