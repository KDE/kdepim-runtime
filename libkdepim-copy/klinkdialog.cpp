/**
 * klinkdialog
 *
 * Copyright 2008  Stephen Kelly <steveire@gmailcom>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "klinkdialog.h"

#include <KLocale>
#include <KLineEdit>
#include <KUrl>

#include <QLabel>
#include <QGridLayout>

KLinkDialog::KLinkDialog(QWidget *parent)
  : KDialog(parent)
{
    setMinimumWidth(450);
    setCaption(i18n("Manage Link"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    setModal(true);

    QWidget *entries = new QWidget();

    QGridLayout *layout = new QGridLayout(entries);

    textLabel = new QLabel(i18n("Link Text"), this);
    textLineEdit = new KLineEdit(this);
    linkUrlLabel = new QLabel(i18n("Link URL"), this);
    linkUrlLineEdit = new KLineEdit(this);

    layout->addWidget(textLabel, 0, 0);
    layout->addWidget(textLineEdit, 0, 1);
    layout->addWidget(linkUrlLabel, 1, 0);
    layout->addWidget(linkUrlLineEdit, 1, 1);

    setMainWidget(entries);

    textLineEdit->setFocus();
}

KLinkDialog::~KLinkDialog()
{}

void KLinkDialog::setLinkText(const QString &linkText)
{
    textLineEdit->setText(linkText);
    if (!linkText.trimmed().isEmpty())
        linkUrlLineEdit->setFocus();
}

void KLinkDialog::setLinkUrl(const QString &linkUrl)
{
    linkUrlLineEdit->setText(linkUrl);
}


QString KLinkDialog::linkText() const
{
    return textLineEdit->text().trimmed();
}

QString KLinkDialog::linkUrl() const
{
    return linkUrlLineEdit->text();
}
