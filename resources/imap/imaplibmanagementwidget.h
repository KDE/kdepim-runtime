/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <tomalbers@kde.nl>

    Based on KMail code by:
    Copyright (C) 2001-2003 Marc Mutz <mutz@kde.org>

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

#ifndef IMAPLIB_MANAGEMENTWIDGET_H
#define IMAPLIB_MANAGEMENTWIDGET_H

#include <imaplib_export.h>
#include <QtGui/QWidget>

/**
  A widget to manage imaplib
*/
class IMAPLIB_EXPORT ImaplibManagementWidget : public QWidget
{
  Q_OBJECT

  public:
    /**
      Creates a new ImaplibManagementWidget.
      @param parent The parent widget.
    */
    ImaplibManagementWidget( QWidget *parent = 0 );

    /**
      Destroys the widget.
    */
    virtual ~ImaplibManagementWidget();

  private Q_SLOTS:
    void fillImaplibList();
    void updateButtonState();
    void addClicked( QAction* );
    void editClicked();
    void removeClicked();

  private:
    class Private;
    Private * const d;
};

#endif
