/*
    SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <MailTransport/TransportAbstractPlugin>
#include <QVariant>
namespace Akonadi
{
class AgentType;
}

class AkonadiMailTransportPlugin : public MailTransport::TransportAbstractPlugin
{
    Q_OBJECT
public:
    explicit AkonadiMailTransportPlugin(QObject *parent = nullptr, const QList<QVariant> & = {});
    ~AkonadiMailTransportPlugin() override;

    [[nodiscard]] QList<MailTransport::TransportAbstractPluginInfo> names() const override;
    [[nodiscard]] bool configureTransport(const QString &identifier, MailTransport::Transport *transport, QWidget *parent) override;

    void cleanUp(MailTransport::Transport *t) override;
    MailTransport::TransportJob *createTransportJob(MailTransport::Transport *t, const QString &identifier) override;
    void initializeTransport(MailTransport::Transport *t, const QString &identifier) override;

private:
    void slotUpdatePluginList(const Akonadi::AgentType &type);
};
