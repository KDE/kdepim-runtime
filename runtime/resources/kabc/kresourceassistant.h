/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KRESOURCEASSISTANT
#define KRESOURCEASSISTANT

#include <kassistantdialog.h>

namespace KRES {
  class Resource;
}

class KResourceAssistant : public KAssistantDialog
{
  Q_OBJECT

  public:
    explicit KResourceAssistant( const QString& resourceFamily, QWidget *parent = 0 );

    virtual ~KResourceAssistant();

    KRES::Resource *resource();

  Q_SIGNALS:
    void error( const QString& message );

  public Q_SLOTS:
    virtual void back();
    virtual void next();

  private:
    class Private;
    Private *d;

    Q_PRIVATE_SLOT( d, void setReadOnly( bool ) )
    Q_PRIVATE_SLOT( d, void slotNameChanged( const QString& ) )
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
