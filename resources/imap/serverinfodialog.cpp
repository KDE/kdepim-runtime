/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "serverinfodialog.h"
#include "imapresource.h"

#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

ServerInfoDialog::ServerInfoDialog(ImapResourceBase *parentResource, QWidget *parent)
    : QDialog(parent)
    , mTextBrowser(new ServerInfoTextBrowser(this))
{
    setWindowTitle(i18nc("@title:window Dialog title for dialog showing information about a server", "Server Info"));
    auto mainLayout = new QVBoxLayout(this);
    setAttribute(Qt::WA_DeleteOnClose);

    mTextBrowser->setPlainText(parentResource->serverCapabilities().join(QLatin1Char('\n')));
    mainLayout->addWidget(mTextBrowser);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ServerInfoDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ServerInfoDialog::reject);
    mainLayout->addWidget(buttonBox);
}

ServerInfoDialog::~ServerInfoDialog()
{
}

ServerInfoTextBrowser::ServerInfoTextBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
}

ServerInfoTextBrowser::~ServerInfoTextBrowser()
{
}

void ServerInfoTextBrowser::paintEvent(QPaintEvent *event)
{
    if (document()->isEmpty()) {
        QPainter p(viewport());

        QFont font = p.font();
        font.setItalic(true);
        p.setFont(font);

        const QPalette palette = viewport()->palette();
        QColor color = palette.text().color();
        color.setAlpha(128);
        p.setPen(color);

        p.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, i18n("Resource not synchronized yet."));
    } else {
        QTextBrowser::paintEvent(event);
    }
}
