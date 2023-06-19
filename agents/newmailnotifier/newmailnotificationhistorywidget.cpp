/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorywidget.h"
#include "newmailnotificationhistorymanager.h"
#include "newmailnotificationhistoryplaintextedit.h"
#include <KLocalizedString>
#include <QVBoxLayout>

NewMailNotificationHistoryWidget::NewMailNotificationHistoryWidget(QWidget *parent)
    : QWidget{parent}
    , mPlainTextEdit(new NewMailNotificationHistoryPlainTextEdit(this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});

    mPlainTextEdit->setObjectName(QStringLiteral("mPlainTextEdit"));
    mPlainTextEdit->setReadOnly(true);
    mainLayout->addWidget(mPlainTextEdit);
    connect(NewMailNotificationHistoryManager::self(),
            &NewMailNotificationHistoryManager::historyAdded,
            this,
            &NewMailNotificationHistoryWidget::slotHistoryAdded);

    mPlainTextEdit->setPlainText(NewMailNotificationHistoryManager::self()->history().join(QLatin1Char('\n')));
}

NewMailNotificationHistoryWidget::~NewMailNotificationHistoryWidget() = default;

void NewMailNotificationHistoryWidget::slotHistoryAdded(const QString &str)
{
    mPlainTextEdit->appendPlainText(str);
}
