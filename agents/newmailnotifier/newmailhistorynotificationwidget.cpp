/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailhistorynotificationwidget.h"
#include <KLocalizedString>
#include <QVBoxLayout>

NewMailHistoryNotificationWidget::NewMailHistoryNotificationWidget(QWidget *parent)
    : QWidget{parent}
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});
}

NewMailHistoryNotificationWidget::~NewMailHistoryNotificationWidget() = default;
