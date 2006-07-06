/*
    This file is part of Akonadi.

    Copyright (c) 2006 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/
#ifndef DATAPROVIDER_H
#define DATAPROVIDER_H

#include "stripeview.h"

#include <QObject>

class AgendaModel;
class ContactModel;

class DataProvider : public QObject
{
    Q_OBJECT
  public:
    DataProvider();

    StripeView::Model *model() const;

    void setupEventData();
    void setupContactData();
    void setupDummyData();

    void load();
    void save();

  protected:
    void addEvent( int year, int month, int day,
      int startHour, int endHour, const QString &title );
    
    void addContact( const QString &name, const QString &email = QString(),
      const QString &phone = QString() );

    void loadFile();
    void saveFile();

    void loadAkonadi();
    void saveAkonadi();

  private:
    StripeView::Model *mModel;
  
    StripeView *mStripe;    
    AgendaModel *mAgendaModel;
    ContactModel *mContactModel;    
};

#endif
