/*
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "serverinfodialog.h"
#include "imapresource_debug.h"

#include <Akonadi/ServerManager>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDialogButtonBox>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>

namespace
{
static const char myServerInfoDialogConfigGroupName[] = "ServerInfoDialog";
}

using namespace Qt::StringLiterals;

ServerInfoDialog::ServerInfoDialog(const QString &identifier, QWidget *parent)
    : QDialog(parent)
    , mTextBrowser(new ServerInfoTextBrowser(this))
{
    setWindowTitle(i18nc("@title:window Dialog title for dialog showing information about a server", "Server Info"));
    auto mainLayout = new QVBoxLayout(this);
    setAttribute(Qt::WA_DeleteOnClose);

    const QString service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Resource, identifier);
    QDBusInterface iface(service, QStringLiteral("/"), QStringLiteral("org.kde.Akonadi.ImapResourceBase"), QDBusConnection::sessionBus(), this);
    if (!iface.isValid()) {
        qCDebug(IMAPRESOURCE_LOG) << "Cannot create imap dbus interface for service " << service;
        deleteLater();
        return;
    }
    QDBusPendingCall call = iface.asyncCall(u"serverCapabilities"_s, (qlonglong)parent->winId());
    auto watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<QStringList> reply = *call;
        if (reply.isError()) {
            mTextBrowser->setPlainText(i18nc("@info:status", "Error while getting server capabilities: %1", reply.error().message()));
        } else {
            mTextBrowser->setPlainText(reply.argumentAt<0>().join(u'\n'));
        }
        call->deleteLater();
    });

    mainLayout->addWidget(mTextBrowser);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ServerInfoDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ServerInfoDialog::reject);
    mainLayout->addWidget(buttonBox);
    readConfig();
}

ServerInfoDialog::~ServerInfoDialog()
{
    writeConfig();
}

void ServerInfoDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myServerInfoDialogConfigGroupName));
    KWindowConfig::saveWindowSize(windowHandle(), group);
}

void ServerInfoDialog::readConfig()
{
    create(); // ensure a window is created
    windowHandle()->resize(QSize(500, 300));
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myServerInfoDialogConfigGroupName));
    KWindowConfig::restoreWindowSize(windowHandle(), group);
    resize(windowHandle()->size()); // workaround for QTBUG-40584
}

ServerInfoTextBrowser::ServerInfoTextBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
}

ServerInfoTextBrowser::~ServerInfoTextBrowser() = default;

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

#include "moc_serverinfodialog.cpp"
