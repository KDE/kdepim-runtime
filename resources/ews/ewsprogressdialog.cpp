/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsprogressdialog.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

EwsProgressDialog::EwsProgressDialog(QWidget *parent, Type type)
    : QDialog(parent)
{
    setModal(true);
    QLabel *statusLabel = new QLabel(this);

    QProgressBar *progress = new QProgressBar(this);
    progress->setMaximum(0);

    QVBoxLayout *vLayout = new QVBoxLayout(this);

    vLayout->setContentsMargins(0, 0, 0, 0);

    vLayout->addWidget(statusLabel);
    vLayout->addWidget(progress);

    QPushButton *cancelButton = new QPushButton(this);
    cancelButton->setText(i18n("Cancel"));

    QWidget *progressContainer = new QWidget(this);
    progressContainer->setLayout(vLayout);

    QHBoxLayout *hLayout = new QHBoxLayout(this);

    hLayout->addWidget(progressContainer);
    hLayout->addWidget(cancelButton);

    hLayout->setSizeConstraint(QLayout::SetFixedSize);

    switch (type) {
    case AutoDiscovery:
        setWindowTitle(i18nc("@title:window", "Exchange server autodiscovery"));
        statusLabel->setText(i18n("Performing Microsoft Exchange server autodiscovery..."));
        break;
    case TryConnect:
        setWindowTitle(i18nc("@title:window", "Connecting to Exchange"));
        statusLabel->setText(i18n("Connecting to Microsoft Exchange server..."));
        break;
    }

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

EwsProgressDialog::~EwsProgressDialog()
{
}
