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

  This widget gives a complete widget which the user can use to configure
  Akonadi resources. It can add, modify or delete resources. With the @p filter
  parameter, you can set which mimetypes should be shown. It also limits the resources
  which can be added by the user.

  Example:

  \code
          tabWidget->addTab(  KCModuleLoader::loadModule(  "kcm_akonadi_resources",
                              KCModuleLoader::Inline, this, QStringList( "message/rfc822" ) ),
                              i18n(  "Mail Servers" ) );
  \endcode

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
    ResourcesManagementWidget( QWidget *parent = 0, const QStringList &filter=QStringList() );

    /**
      Destroys the widget.
    */
    virtual ~ResourcesManagementWidget();

  private Q_SLOTS:
    void fillResourcesList();
    void updateButtonState();
    void addClicked( QAction* );
    void editClicked();
    void removeClicked();

  private:
    class Private;
    Private * const d;
};

#endif
