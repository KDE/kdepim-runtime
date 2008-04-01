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

class KLinkDialogPrivate
{
    public:
        QLabel *textLabel;
        KLineEdit *textLineEdit;
        QLabel *linkUrlLabel;
        KLineEdit *linkUrlLineEdit;
};

KLinkDialog::KLinkDialog(QWidget *parent)
  : KDialog(parent), d(new KLinkDialogPrivate)
{
    setCaption(i18n("Manage Link"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    setModal(true);

    QWidget *entries = new QWidget();

    QGridLayout *layout = new QGridLayout(entries);

    d->textLabel = new QLabel(i18n("Link Text"), this);
    d->textLineEdit = new KLineEdit(this);
    d->linkUrlLabel = new QLabel(i18n("Link URL"), this);
    d->linkUrlLineEdit = new KLineEdit(this);

    layout->addWidget(d->textLabel, 0, 0);
    layout->addWidget(d->textLineEdit, 0, 1);
    layout->addWidget(d->linkUrlLabel, 1, 0);
    layout->addWidget(d->linkUrlLineEdit, 1, 1);

    setMainWidget(entries);

    KConfigGroup group(KGlobal::config(), "KLinkDialog");
    restoreDialogSize(group);

    d->textLineEdit->setFocus();
}

KLinkDialog::~KLinkDialog()
{
    KConfigGroup group(KGlobal::config(), "KLinkDialog");
    saveDialogSize(group);
    delete d;
    d = 0;
}

void KLinkDialog::setLinkText(const QString &linkText)
{
    d->textLineEdit->setText(linkText);
    if (!linkText.trimmed().isEmpty())
        d->linkUrlLineEdit->setFocus();
}

void KLinkDialog::setLinkUrl(const QString &linkUrl)
{
    d->linkUrlLineEdit->setText(linkUrl);
}


QString KLinkDialog::linkText() const
{
    return d->textLineEdit->text().trimmed();
}

QString KLinkDialog::linkUrl() const
{
    return d->linkUrlLineEdit->text();
}
