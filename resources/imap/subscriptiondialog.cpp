/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "subscriptiondialog.h"

#include <QtCore/QCoreApplication>
#include <QStandardItemModel>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QCheckBox>

#include "imapresource_debug.h"
#include <QLineEdit>
#include <KSharedConfig>

#include <KLocalizedString>

#include <kimap/session.h>
#include <kimap/loginjob.h>
#include <kimap/unsubscribejob.h>
#include <kimap/subscribejob.h>

#include "imapaccount.h"
#include "sessionuiproxy.h"
#include <QPushButton>
#include <QDialogButtonBox>
#include <KConfigGroup>

#include <QHeaderView>
#include <QLabel>
#include <QTreeView>

SubscriptionDialog::SubscriptionDialog(QWidget *parent, SubscriptionDialog::SubscriptionDialogOptions option)
    : QDialog(parent),
      m_session(nullptr),
      m_subscriptionChanged(false),
      m_lineEdit(nullptr),
      m_filter(new SubscriptionFilterProxyModel(this)),
      m_model(new QStandardItemModel(this))
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    setModal(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    mUser1Button = new QPushButton;
    buttonBox->addButton(mUser1Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SubscriptionDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SubscriptionDialog::reject);

    mUser1Button->setText(i18nc("@action:button", "Reload &List"));
    mUser1Button->setEnabled(false);
    connect(mUser1Button, &QPushButton::clicked, this, &SubscriptionDialog::onReloadRequested);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setMargin(0);
    mainWidget->setLayout(mainLayout);
    topLayout->addWidget(mainWidget);
    topLayout->addWidget(buttonBox);

    m_enableSubscription = new QCheckBox(i18nc("@option:check",
                                         "Enable server-side subscriptions"));
    mainLayout->addWidget(m_enableSubscription);

    QHBoxLayout *filterBarLayout = new QHBoxLayout;
    mainLayout->addLayout(filterBarLayout);

    filterBarLayout->addWidget(new QLabel(i18nc("@label search for a subscription",
                                          "Search:")));

    m_lineEdit = new QLineEdit(mainWidget);
    m_lineEdit->setClearButtonEnabled(true);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &SubscriptionDialog::slotSearchPattern);
    filterBarLayout->addWidget(m_lineEdit);
    m_lineEdit->setFocus();

    QCheckBox *checkBox = new QCheckBox(i18nc("@option:check", "Subscribed only"), mainWidget);
    connect(checkBox, SIGNAL(stateChanged(int)),
            m_filter, SLOT(setIncludeCheckedOnly(int)));

    filterBarLayout->addWidget(checkBox);

    m_treeView = new QTreeView(mainWidget);
    m_treeView->header()->hide();
    m_filter->setSourceModel(m_model);
    m_treeView->setModel(m_filter);
    mainLayout->addWidget(m_treeView);

    connect(m_model, &QStandardItemModel::itemChanged, this, &SubscriptionDialog::onItemChanged);

    if (option & SubscriptionDialog::AllowToEnableSubscription) {
        connect(m_enableSubscription, &QCheckBox::clicked, m_treeView, &QTreeView::setEnabled);
    } else {
        m_enableSubscription->hide();
    }
    readConfig();
}

SubscriptionDialog::~SubscriptionDialog()
{
    writeConfig();
}

void SubscriptionDialog::slotSearchPattern(const QString &pattern)
{
    m_treeView->expandAll();
    m_filter->setSearchPattern(pattern);
}

void SubscriptionDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "SubscriptionDialog");

    const QSize size = group.readEntry("Size", QSize());
    if (size.isValid()) {
        resize(size);
    } else {
        resize(500, 300);
    }
}

void SubscriptionDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "SubscriptionDialog");
    group.writeEntry("Size", size());
    group.sync();
}

void SubscriptionDialog::setSubscriptionEnabled(bool enabled)
{
    m_enableSubscription->setChecked(enabled);
    m_treeView->setEnabled(enabled);
}

bool SubscriptionDialog::subscriptionEnabled() const
{
    return m_enableSubscription->isChecked();
}

void SubscriptionDialog::connectAccount(const ImapAccount &account, const QString &password)
{
    m_session = new KIMAP::Session(account.server(), account.port(), this);
    m_session->setUiProxy(SessionUiProxy::Ptr(new SessionUiProxy));

    KIMAP::LoginJob *login = new KIMAP::LoginJob(m_session);
    login->setUserName(account.userName());
    login->setPassword(password);
    login->setEncryptionMode(account.encryptionMode());
    login->setAuthenticationMode(account.authenticationMode());

    connect(login, &KIMAP::LoginJob::result, this, &SubscriptionDialog::onLoginDone);
    login->start();
}

bool SubscriptionDialog::isSubscriptionChanged() const
{
    return m_subscriptionChanged;
}

void SubscriptionDialog::onLoginDone(KJob *job)
{
    if (!job->error()) {
        onReloadRequested();
    }
}

void SubscriptionDialog::onReloadRequested()
{
    mUser1Button->setEnabled(false);
    m_itemsMap.clear();
    m_model->clear();

    // we need a connection
    if (!m_session
            || m_session->state() != KIMAP::Session::Authenticated) {
        qCWarning(IMAPRESOURCE_LOG) << "SubscriptionDialog - got no connection";
        mUser1Button->setEnabled(true);
        return;
    }

    KIMAP::ListJob *list = new KIMAP::ListJob(m_session);
    list->setOption(KIMAP::ListJob::IncludeUnsubscribed);
    connect(list, &KIMAP::ListJob::mailBoxesReceived, this, &SubscriptionDialog::onMailBoxesReceived);
    connect(list, &KIMAP::ListJob::result, this, &SubscriptionDialog::onFullListingDone);
    list->start();
}

void SubscriptionDialog::onMailBoxesReceived(const QList<KIMAP::MailBoxDescriptor> &mailBoxes,
        const QList< QList<QByteArray> > &flags)
{
    const int numberOfMailBoxes(mailBoxes.size());
    for (int i = 0; i < numberOfMailBoxes; i++) {
        KIMAP::MailBoxDescriptor mailBox = mailBoxes[i];

        const QStringList pathParts = mailBox.name.split(mailBox.separator);
        const QString separator = mailBox.separator;
        Q_ASSERT(separator.size() == 1);   // that's what the spec says

        QString parentPath;
        QString currentPath;
        const int numberOfPath(pathParts.size());
        for (int j = 0; j < pathParts.size(); ++j) {
            const bool isDummy = (j != (numberOfPath - 1));
            const bool isCheckable = !isDummy && !flags[i].contains("\\noselect");

            const QString pathPart = pathParts.at(j);
            currentPath += separator + pathPart;

            if (m_itemsMap.contains(currentPath)) {
                if (!isDummy) {
                    QStandardItem *item = m_itemsMap[currentPath];
                    item->setCheckable(isCheckable);
                }

            } else if (!parentPath.isEmpty()) {
                Q_ASSERT(m_itemsMap.contains(parentPath));

                QStandardItem *parentItem = m_itemsMap[parentPath];

                QStandardItem *item = new QStandardItem(pathPart);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                item->setCheckable(isCheckable);
                item->setData(currentPath.mid(1), PathRole);
                parentItem->appendRow(item);
                m_itemsMap[currentPath] = item;

            } else {
                QStandardItem *item = new QStandardItem(pathPart);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                item->setCheckable(isCheckable);
                item->setData(currentPath.mid(1), PathRole);
                m_model->appendRow(item);
                m_itemsMap[currentPath] = item;
            }

            parentPath = currentPath;
        }
    }
}

void SubscriptionDialog::onFullListingDone(KJob *job)
{
    if (job->error()) {
        mUser1Button->setEnabled(true);
        return;
    }

    KIMAP::ListJob *list = new KIMAP::ListJob(m_session);
    list->setOption(KIMAP::ListJob::NoOption);
    connect(list, &KIMAP::ListJob::mailBoxesReceived, this, &SubscriptionDialog::onSubscribedMailBoxesReceived);
    connect(list, &KIMAP::ListJob::result, this, &SubscriptionDialog::onReloadDone);
    list->start();
}

void SubscriptionDialog::onSubscribedMailBoxesReceived(const QList<KIMAP::MailBoxDescriptor> &mailBoxes,
        const QList< QList<QByteArray> > &flags)
{
    Q_UNUSED(flags);
    const int numberOfMailBoxes(mailBoxes.size());
    for (int i = 0; i < numberOfMailBoxes; ++i) {
        KIMAP::MailBoxDescriptor mailBox = mailBoxes.at(i);
        QString descriptor = mailBox.separator + mailBox.name;

        if (m_itemsMap.contains(descriptor)) {
            QStandardItem *item = m_itemsMap[mailBox.separator + mailBox.name];
            item->setCheckState(Qt::Checked);
            item->setData(Qt::Checked, InitialStateRole);
        }
    }
}

void SubscriptionDialog::onReloadDone(KJob *job)
{
    Q_UNUSED(job);
    mUser1Button->setEnabled(true);
}

void SubscriptionDialog::onItemChanged(QStandardItem *item)
{
    QFont font = item->font();
    font.setBold(item->checkState() != item->data(InitialStateRole).toInt());
    item->setFont(font);
}

void SubscriptionDialog::slotAccepted()
{
    applyChanges();
    accept();
}

void SubscriptionDialog::applyChanges()
{
    QList<QStandardItem *> items = m_itemsMap.values();

    while (!items.isEmpty()) {
        QStandardItem *item = items.takeFirst();

        if (item->checkState() != item->data(InitialStateRole).toInt()) {
            if (item->checkState() == Qt::Checked) {
                qCDebug(IMAPRESOURCE_LOG) << "Subscribing" << item->data(PathRole);
                KIMAP::SubscribeJob *subscribe = new KIMAP::SubscribeJob(m_session);
                subscribe->setMailBox(item->data(PathRole).toString());
                subscribe->exec();
            } else {
                qCDebug(IMAPRESOURCE_LOG) << "Unsubscribing" << item->data(PathRole);
                KIMAP::UnsubscribeJob *unsubscribe = new KIMAP::UnsubscribeJob(m_session);
                unsubscribe->setMailBox(item->data(PathRole).toString());
                unsubscribe->exec();
            }

            m_subscriptionChanged = true;
        }
    }
}

SubscriptionFilterProxyModel::SubscriptionFilterProxyModel(QObject *parent)
    : KRecursiveFilterProxyModel(parent), m_checkedOnly(false)
{

}

void SubscriptionFilterProxyModel::setSearchPattern(const QString &pattern)
{
    if (m_pattern != pattern) {
        m_pattern = pattern;
        invalidate();
    }
}

void SubscriptionFilterProxyModel::setIncludeCheckedOnly(bool checkedOnly)
{
    if (m_checkedOnly != checkedOnly) {
        m_checkedOnly = checkedOnly;
        invalidate();
    }
}

void SubscriptionFilterProxyModel::setIncludeCheckedOnly(int checkedOnlyState)
{
    m_checkedOnly = (checkedOnlyState == Qt::Checked);
    invalidate();
}

bool SubscriptionFilterProxyModel::acceptRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    const bool checked = sourceIndex.data(Qt::CheckStateRole).toInt() == Qt::Checked;

    if (m_checkedOnly && !checked) {
        return false;
    } else if (!m_pattern.isEmpty()) {
        const QString text = sourceIndex.data(Qt::DisplayRole).toString();
        return text.contains(m_pattern, Qt::CaseInsensitive);
    } else {
        return true;
    }
}
