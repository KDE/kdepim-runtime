/*
    SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include "config-newmailnotifier.h"
#include <agentbase.h>
#include <collection.h> // make sure this is included before QHash, otherwise it won't find the correct qHash implementation for some reason

#include <QPixmap>
#include <QStringList>
#include <QTimer>
class QTextToSpeech;

namespace Akonadi
{
class AgentInstance;
}

namespace KIdentityManagement
{
class IdentityManager;
}

class NewMailNotifierAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit NewMailNotifierAgent(const QString &id);

    void setEnableAgent(bool b);
    Q_REQUIRED_RESULT bool enabledAgent() const;

    void printDebug();

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection) override;
    void itemsRemoved(const Akonadi::Item::List &items) override;
    void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags) override;
    void doSetOnline(bool online) override;

private:
    void slotShowNotifications();
    void slotInstanceStatusChanged(const Akonadi::AgentInstance &instance);
    void slotInstanceRemoved(const Akonadi::AgentInstance &instance);
    void slotInstanceAdded(const Akonadi::AgentInstance &instance);
    void slotDisplayNotification(const QPixmap &pixmap, const QString &message);
    void slotIdentitiesChanged();
    void slotInstanceNameChanged(const Akonadi::AgentInstance &instance);
    void slotSay(const QString &message);
    bool excludeAgentType(const Akonadi::AgentInstance &instance);
    bool ignoreStatusMail(const Akonadi::Item &item);
    bool isActive() const;
    void clearAll();
    bool excludeSpecialCollection(const Akonadi::Collection &collection) const;
    void slotReloadConfiguration();
    QString mDefaultIconName;
    QStringList mListEmails;
    QHash<Akonadi::Collection, QList<Akonadi::Item::Id>> mNewMails;
    QHash<QString, QString> mCacheResourceName;
    QTimer mTimer;
    QStringList mInstanceNameInProgress;
    KIdentityManagement::IdentityManager *mIdentityManager = nullptr;
#if HAVE_TEXT_TO_SPEECH_SUPPORT
    QTextToSpeech *mTextToSpeech = nullptr;
#endif
};

