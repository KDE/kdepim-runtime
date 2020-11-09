/*
    SPDX-FileCopyrightText: 2016-2020 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotifierselectcollectionwidget_test.h"
#include "../newmailnotifierselectcollectionwidget.h"
#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    auto *w = new NewMailNotifierSelectCollectionWidget;
    w->show();
    app.exec();
    delete w;
    return 0;
}
