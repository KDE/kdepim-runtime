/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailhistorynotificationdialog.h"
#include "newmailhistorynotificationwidget.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QWindow>

namespace
{
static const char myConfigNewMailHistoryNotificationDialogGroupName[] = "NewMailHistoryNotificationDialog";
}
NewMailHistoryNotificationDialog::NewMailHistoryNotificationDialog(QWidget *parent)
    : QDialog(parent)
    , mNewHistoryNotificationWidget(new NewMailHistoryNotificationWidget(this))
{
    setWindowTitle(i18nc("@title:window", "Notification History"));
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));

    mNewHistoryNotificationWidget->setObjectName(QStringLiteral("mNewHistoryNotificationWidget"));
    mainLayout->addWidget(mNewHistoryNotificationWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewMailHistoryNotificationDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewMailHistoryNotificationDialog::reject);
    readConfig();
}

NewMailHistoryNotificationDialog::~NewMailHistoryNotificationDialog()
{
    writeConfig();
}

void NewMailHistoryNotificationDialog::readConfig()
{
    create(); // ensure a window is created
    windowHandle()->resize(QSize(800, 300));
    KConfigGroup group(KSharedConfig::openStateConfig(), myConfigNewMailHistoryNotificationDialogGroupName);
    KWindowConfig::restoreWindowSize(windowHandle(), group);
    resize(windowHandle()->size()); // workaround for QTBUG-40584
}

void NewMailHistoryNotificationDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), myConfigNewMailHistoryNotificationDialogGroupName);
    KWindowConfig::saveWindowSize(windowHandle(), group);
}
