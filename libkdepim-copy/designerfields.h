/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef KPIM_DESIGNERFIELDS_H
#define KPIM_DESIGNERFIELDS_H

#include <klocale.h>

#include <qmap.h>
#include <qpair.h>
#include <qstringlist.h>

namespace KPIM {

class DesignerFields : public QWidget
{
    Q_OBJECT
  public:
    DesignerFields( const QString &uiFile, QWidget *parent,
      const char *name = 0 );

    class Storage
    {
      public:
        virtual ~Storage() {}
      
        virtual QStringList keys() = 0;
        virtual QString read( const QString &key ) = 0;
        virtual void write( const QString &key, const QString &value ) = 0;
    };

    void load( Storage * );
    void save( Storage * );

    void setReadOnly( bool readOnly );

    QString identifier() const;
    QString title() const;

  signals:
    void modified();

  private:
    void initGUI( const QString& );

    QMap<QString, QWidget *> mWidgets;
    QValueList<QWidget *> mDisabledWidgets;
    QString mTitle;
    QString mIdentifier;
};

}

#endif
