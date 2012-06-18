/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GOOGLE_CALENDAR_CALENDAREDITOR_H
#define GOOGLE_CALENDAR_CALENDAREDITOR_H

#include <QDialog>

#include <libkgapi/objects/calendar.h>

namespace Ui {
  class CalendarEditor;
}

using namespace KGAPI::Objects;

class CalendarEditor: public QDialog
{
  Q_OBJECT
  public:
    explicit CalendarEditor( Calendar *calendar = 0 );

    virtual ~CalendarEditor();

  Q_SIGNALS:
    void accepted( KGAPI::Objects::Calendar *calendar );

  private Q_SLOTS:
    void accepted();

  private:
    void initTimezones();
    void initColors();
    void initWidgets();

    Calendar *m_calendar;
    Ui::CalendarEditor *m_ui;
};

#endif /* CALENDAREDITOR_H */

