/*
 * Copyright (c) 2009  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "script.h"
#include "global.h"
#include <QDebug>
#include <qcoreapplication.h>

Script::Script()
{
    action = new Kross::Action(this, "ResourceTester");
    connect(action, &Kross::Action::finished, this, &Script::finished);
    action->addObject(this, QStringLiteral("Script"));
}

void Script::configure(const QString &path)
{
    action->setFile(path);
}

void Script::insertObject(QObject *object, const QString &objectName)
{
    action->addObject(object, objectName);
}

void Script::include(const QString &path)
{
    QFile f(Global::basePath() + path);
    if (!f.open(QFile::ReadOnly)) {
        qCritical() << "Unable to open file" << Global::basePath() + path;
    } else {
        action->evaluate(f.readAll());
    }
}

QString Script::absoluteFileName(const QString &path)
{
    return Global::basePath() + path;
}

void Script::start()
{
    action->trigger();
}

void Script::finished(Kross::Action *action)
{
    if (action->hadError()) {
        qCritical() << action->errorMessage() << action->errorTrace();
        QCoreApplication::instance()->exit(1);
    } else {
        QCoreApplication::instance()->quit();
    }
}

