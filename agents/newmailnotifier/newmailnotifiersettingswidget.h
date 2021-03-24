/*
    SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/AgentConfigurationBase>
#include <AkonadiCore/Collection>

class KNotifyConfigWidget;
class QCheckBox;
class QLineEdit;
class NewMailNotifierSelectCollectionWidget;
class NewMailNotifierSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit NewMailNotifierSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~NewMailNotifierSettingsWidget() override;

    void load() override;
    bool save() const override;

private:
    void slotHelpLinkClicked(const QString &);
    QCheckBox *mShowPhoto = nullptr;
    QCheckBox *mShowFrom = nullptr;
    QCheckBox *mShowSubject = nullptr;
    QCheckBox *mShowFolders = nullptr;
    QCheckBox *mExcludeMySelf = nullptr;
    QCheckBox *mAllowToShowMail = nullptr;
    QCheckBox *mKeepPersistentNotification = nullptr;
    KNotifyConfigWidget *mNotify = nullptr;
    QCheckBox *mTextToSpeak = nullptr;
    QLineEdit *mTextToSpeakSetting = nullptr;
    NewMailNotifierSelectCollectionWidget *mSelectCollection = nullptr;
};

AKONADI_AGENTCONFIG_FACTORY(NewMailNotifierSettingsFactory, "newmailnotifierconfig.json", NewMailNotifierSettingsWidget)

