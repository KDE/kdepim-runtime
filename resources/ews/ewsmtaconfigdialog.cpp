/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsmtaconfigdialog.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QPushButton>

#include <KWindowSystem>

#include <AkonadiCore/AgentFilterProxyModel>
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentInstanceModel>
#include <AkonadiWidgets/AgentInstanceWidget>

#include "ewsmtaresource.h"
#include "ewsmtasettings.h"
#include "ewsres_mta_debug.h"
#include "ui_ewsmtaconfigdialog.h"

EwsMtaConfigDialog::EwsMtaConfigDialog(EwsMtaResource *parentResource, WId wId)
    : QDialog()
    , mParentResource(parentResource)
{
    if (wId) {
        setAttribute(Qt::WA_NativeWindow, true);
        KWindowSystem::setMainWindow(windowHandle(), wId);
    }

    QWidget *mainWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(this);
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &EwsMtaConfigDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &EwsMtaConfigDialog::reject);
    mainLayout->addWidget(mButtonBox);

    setWindowTitle(i18nc("@title:window", "Microsoft Exchange Mail Transport Configuration"));

    mUi = new Ui::SetupServerView;
    mUi->setupUi(mainWidget);
    mUi->accountName->setText(parentResource->name());

    Akonadi::AgentFilterProxyModel *model = mUi->resourceWidget->agentFilterProxyModel();
    model->addCapabilityFilter(QStringLiteral("X-EwsMailTransport"));
    mUi->resourceWidget->view()->setSelectionMode(QAbstractItemView::SingleSelection);

    for (int i = 0, total = model->rowCount(); i < total; ++i) {
        QModelIndex index = model->index(i, 0);
        QVariant v = model->data(index, Akonadi::AgentInstanceModel::InstanceIdentifierRole);
        if (v.toString() == EwsMtaSettings::ewsResource()) {
            mUi->resourceWidget->view()->setCurrentIndex(index);
        }
    }

    connect(okButton, &QPushButton::clicked, this, &EwsMtaConfigDialog::save);
}

EwsMtaConfigDialog::~EwsMtaConfigDialog()
{
    delete mUi;
}

void EwsMtaConfigDialog::save()
{
    if (!mUi->resourceWidget->selectedAgentInstances().isEmpty()) {
        EwsMtaSettings::setEwsResource(mUi->resourceWidget->selectedAgentInstances().constFirst().identifier());
        mParentResource->setName(mUi->accountName->text());
        EwsMtaSettings::self()->save();
    } else {
        qCWarning(EWSRES_MTA_LOG) << "Any agent instance selected";
    }
}
