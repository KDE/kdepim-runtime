/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailhistorynotificationwidget.h"
#include "newmailhistorynotificationmanager.h"
#include "newmailnotificationhistoryplaintextedit.h"
#include <KLocalizedString>
#include <QVBoxLayout>

NewMailHistoryNotificationWidget::NewMailHistoryNotificationWidget(QWidget *parent)
    : QWidget{parent}
    , mPlainTextEdit(new NewMailNotificationHistoryPlainTextEdit(this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});

    mPlainTextEdit->setObjectName(QStringLiteral("mPlainTextEdit"));
    mPlainTextEdit->setReadOnly(true);
    mainLayout->addWidget(mPlainTextEdit);
    connect(NewMailHistoryNotificationManager::self(),
            &NewMailHistoryNotificationManager::historyAdded,
            this,
            &NewMailHistoryNotificationWidget::slotHistoryAdded);

    mPlainTextEdit->setPlainText(NewMailHistoryNotificationManager::self()->history().join(QLatin1Char('\n')));
}

NewMailHistoryNotificationWidget::~NewMailHistoryNotificationWidget() = default;

void NewMailHistoryNotificationWidget::slotHistoryAdded(const QString &str)
{
    mPlainTextEdit->appendPlainText(str);
}
