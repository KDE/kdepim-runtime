/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailhistorynotificationdialog.h"
#include "newmailhistorynotificationwidget.h"
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QVBoxLayout>

NewMailHistoryNotificationDialog::NewMailHistoryNotificationDialog(QWidget *parent)
    : QDialog(parent)
    , mNewHistoryNotificationWidget(new NewMailHistoryNotificationWidget(this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));

    mNewHistoryNotificationWidget->setObjectName(QStringLiteral("mNewHistoryNotificationWidget"));
    mainLayout->addWidget(mNewHistoryNotificationWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewMailHistoryNotificationDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewMailHistoryNotificationDialog::reject);
}

NewMailHistoryNotificationDialog::~NewMailHistoryNotificationDialog() = default;
