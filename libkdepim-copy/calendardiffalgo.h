/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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

#ifndef KPIM_CALENDARDIFFALGO_H
#define KPIM_CALENDARDIFFALGO_H

#include <libkcal/event.h>
#include <libkcal/todo.h>
#include <libkdepim/diffalgo.h>

namespace KPIM {

class KDE_EXPORT CalendarDiffAlgo : public DiffAlgo
{
  public:
    CalendarDiffAlgo( KCal::Incidence *leftIncidence, KCal::Incidence *rightIncidence );

    void run();

  private:
    template <class L>
    void diffList( const QString &id, const QValueList<L> &left, const QValueList<L> &right );

    void diffIncidenceBase( KCal::IncidenceBase*, KCal::IncidenceBase* );
    void diffIncidence( KCal::Incidence*, KCal::Incidence* );
    void diffEvent( KCal::Event*, KCal::Event* );
    void diffTodo( KCal::Todo*, KCal::Todo* );

    KCal::Incidence *mLeftIncidence;
    KCal::Incidence *mRightIncidence;
};

}

#endif
