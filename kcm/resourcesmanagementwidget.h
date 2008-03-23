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

#ifndef RESOURCES_MANAGEMENTWIDGET_H
#define RESOURCES_MANAGEMENTWIDGET_H

#include <QtGui/QWidget>
#include <QStringList>

/**
  @short A widget to manage imaplib
  @author Tom Albers <tomalbers@kde.nl>
*/
class ResourcesManagementWidget : public QWidget
{
    Q_OBJECT

public:
    /**
      @short Creates a new ResourcesManagementWidget.
      @param parent The parent widget.
      @param filter The mimetypes which you want shown in the widget. Leave
                    it empty to see them all.
    */
    explicit ResourcesManagementWidget( QWidget *parent = 0,
                                        const QStringList &filter=QStringList() );

    /**
      Destroys the widget.
    */
    virtual ~ResourcesManagementWidget();

private Q_SLOTS:
    void updateButtonState( const QString& = QString() );
    void addClicked( QAction* );
    void editClicked();
    void removeClicked();

private:
    class Private;
    Private * const d;
};

#endif
