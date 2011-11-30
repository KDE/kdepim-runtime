/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include <QtCore/qcoreapplication.h>
#include <feederqueue.h>
#include <QtCore/QStringList>
#include <akonadi/item.h>

int main(int argc, char *argv[])
 {
     QCoreApplication app(argc, argv);
     if (app.arguments().size() != 2) {
         return -1;
     }
     ItemQueue queue(1, 1, &app);
     Akonadi::Item::Id id = app.arguments().at(1).toInt();
     kDebug() << "indexing item: " << id;
     queue.addItem(Akonadi::Item(id));
     queue.processItem();
     QObject::connect( &queue, SIGNAL(finished()), &app, SLOT(quit()));
     return app.exec();
 }