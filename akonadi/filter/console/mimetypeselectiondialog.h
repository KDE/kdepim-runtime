/******************************************************************************
 *
 *  File : mimetypeselectiondialog.h
 *  Created on Sat 13 Jun 2009 06:08:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _MIMETYPESELECTIONDIALOG_H_
#define _MIMETYPESELECTIONDIALOG_H_

#include <KDialog>

#include <QStringList>

class KComboBox;

class MimeTypeSelectionDialog : public KDialog
{
  Q_OBJECT
public:
  MimeTypeSelectionDialog( QWidget * parent, const QStringList &mimeTypes );
  virtual ~MimeTypeSelectionDialog();
protected:
  KComboBox * mMimeTypeComboBox;
  QString mSelectedMimeType;
public:
  const QString & selectedMimeType() const
  {
    return mSelectedMimeType;
  }
protected slots:
  void slotOkClicked();
};

#endif //!_MIMETYPESELECTIONDIALOG_H_
