/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "setuppage.h"
#include <QIcon>
#include <qstandarditemmodel.h>

SetupPage::SetupPage(KAssistantDialog *parent) :
    Page(parent),
    m_msgModel(new QStandardItemModel(this))
{
    ui.setupUi(this);
    ui.detailView->setModel(m_msgModel);
    connect(ui.detailsButton, &QPushButton::clicked, this, &SetupPage::detailsClicked);
}

void SetupPage::enterPageNext()
{
    ui.stackWidget->setCurrentIndex(0);
}

void SetupPage::detailsClicked()
{
    ui.stackWidget->setCurrentIndex(1);
}

void SetupPage::addMessage(SetupPage::MessageType type, const QString &msg)
{
    QStandardItem *item = new QStandardItem;
    item->setText(msg);
    item->setEditable(false);
    switch (type) {
    case Success:
        item->setIcon(QIcon::fromTheme(QLatin1String("dialog-ok")));
        break;
    case Info:
        item->setIcon(QIcon::fromTheme(QLatin1String("dialog-information")));
        break;
    case Error:
        item->setIcon(QIcon::fromTheme(QLatin1String("dialog-error")));
        break;
    }
    m_msgModel->appendRow(item);
}

void SetupPage::setStatus(const QString &msg)
{
    ui.progressLabel->setText(msg);
}

void SetupPage::setProgress(int percent)
{
    ui.progressBar->setValue(percent);
}

