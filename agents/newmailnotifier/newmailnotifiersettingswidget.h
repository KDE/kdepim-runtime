/*
    Copyright (c) 2013-2018 Laurent Montel <montel@kde.org>

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

#ifndef NEWMAILNOTIFIERSETTINGSWIDGET_H
#define NEWMAILNOTIFIERSETTINGSWIDGET_H

#include <QDialog>

#include <AkonadiCore/Collection>
#include <AkonadiCore/AgentConfigurationBase>

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

#endif // NEWMAILNOTIFIERSETTINGSWIDGET_H
