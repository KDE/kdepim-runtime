/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorywidget.h"
#include "newmailnotificationhistorybrowsertext.h"
#include "newmailnotificationhistorybrowsertextwidget.h"
#include "newmailnotificationhistorymanager.h"
#include "newmailnotifieragentsettings.h"
#include <KLocalizedString>

#include <TextCustomEditor/PlainTextEditor>

#include <QCheckBox>
#include <QScrollBar>
#include <QVBoxLayout>

NewMailNotificationHistoryWidget::NewMailNotificationHistoryWidget(QWidget *parent)
    : QWidget{parent}
    , mTextBrowser(new NewMailNotificationHistoryBrowserTextWidget(new NewMailNotificationHistoryBrowserText(this), this))
    , mEnabledHistory(new QCheckBox(i18n("Enabled"), this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});

    mEnabledHistory->setObjectName(QStringLiteral("mEnabledHistory"));

    mTextBrowser->setObjectName(QStringLiteral("mTextBrowser"));
    mainLayout->addWidget(mTextBrowser);

    mainLayout->addWidget(mEnabledHistory);
    connect(mEnabledHistory, &QCheckBox::clicked, this, &NewMailNotificationHistoryWidget::slotEnableChanged);

    connect(NewMailNotificationHistoryManager::self(),
            &NewMailNotificationHistoryManager::historyAdded,
            this,
            &NewMailNotificationHistoryWidget::slotHistoryAdded);

    mTextBrowser->setHtml(NewMailNotificationHistoryManager::self()->history().join(QStringLiteral("<br>")));
    connect(mTextBrowser, &NewMailNotificationHistoryBrowserTextWidget::clearHistory, this, [this]() {
        NewMailNotificationHistoryManager::self()->clear();
        mTextBrowser->clear();
    });
    slotEnableChanged(NewMailNotifierAgentSettings::self()->enableNotificationHistory());
    mEnabledHistory->setChecked(NewMailNotifierAgentSettings::self()->enableNotificationHistory());
}

NewMailNotificationHistoryWidget::~NewMailNotificationHistoryWidget() = default;

void NewMailNotificationHistoryWidget::slotHistoryAdded(const QString &str)
{
    mTextBrowser->editor()->setHtml(str);
    mTextBrowser->editor()->verticalScrollBar()->setValue(mTextBrowser->editor()->verticalScrollBar()->maximum());
}

void NewMailNotificationHistoryWidget::slotEnableChanged(bool clicked)
{
    mTextBrowser->setEnabled(clicked);
    NewMailNotifierAgentSettings::self()->setEnableNotificationHistory(clicked);
    NewMailNotifierAgentSettings::self()->save();
}

#include "moc_newmailnotificationhistorywidget.cpp"
