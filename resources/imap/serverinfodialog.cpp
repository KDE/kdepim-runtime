/*
  Copyright (c) 2013-2019 Montel Laurent <montel@kde.org>

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

#include "serverinfodialog.h"
#include "imapresource.h"

#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>

ServerInfoDialog::ServerInfoDialog(ImapResourceBase *parentResource, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(
        i18nc("@title:window Dialog title for dialog showing information about a server",
              "Server Info"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setAttribute(Qt::WA_DeleteOnClose);

    mTextBrowser = new ServerInfoTextBrowser(this);
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
