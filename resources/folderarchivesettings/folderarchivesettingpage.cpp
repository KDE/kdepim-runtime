/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "folderarchivesettingpage.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchiveutil.h"

#include <CollectionRequester>

#include <kmime/kmime_message.h>

#include <KLocalizedString>
#include <KSharedConfig>

#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDBusConnectionInterface>

FolderArchiveComboBox::FolderArchiveComboBox(QWidget *parent)
    : QComboBox(parent)
{
    initialize();
}

FolderArchiveComboBox::~FolderArchiveComboBox()
{
}

void FolderArchiveComboBox::initialize()
{
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Unique"),
            FolderArchiveAccountInfo::UniqueFolder);
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Month and year"),
            FolderArchiveAccountInfo::FolderByMonths);
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Year"),
            FolderArchiveAccountInfo::FolderByYears);
}

void FolderArchiveComboBox::setType(FolderArchiveAccountInfo::FolderArchiveType type)
{
    const int index = findData(static_cast<int>(type));
    if (index != -1) {
        setCurrentIndex(index);
    } else {
        setCurrentIndex(0);
    }
}

FolderArchiveAccountInfo::FolderArchiveType FolderArchiveComboBox::type() const
{
    return static_cast<FolderArchiveAccountInfo::FolderArchiveType>(itemData(currentIndex()).toInt());
}

FolderArchiveSettingPage::FolderArchiveSettingPage(const QString &instanceName, QWidget *parent)
    : QWidget(parent)
    , mInstanceName(instanceName)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    mEnabled = new QCheckBox(i18n("Enable"));
    connect(mEnabled, &QCheckBox::toggled, this, &FolderArchiveSettingPage::slotEnableChanged);
    lay->addWidget(mEnabled);

    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *lab = new QLabel(i18nc(
                                 "@label:chooser for the folder that messages will be archived under",
                                 "Archive into:"));
    hbox->addWidget(lab);
    mArchiveFolder = new Akonadi::CollectionRequester(this);
    mArchiveFolder->setMimeTypeFilter(QStringList() << KMime::Message::mimeType());
    hbox->addWidget(mArchiveFolder);
    lay->addLayout(hbox);

    hbox = new QHBoxLayout;
    lab = new QLabel(i18nc("@label:listbox", "Archive folder name:"));
    hbox->addWidget(lab);
    mArchiveNamed = new FolderArchiveComboBox;
    hbox->addWidget(mArchiveNamed);

    lay->addLayout(hbox);

    lay->addStretch();
}

FolderArchiveSettingPage::~FolderArchiveSettingPage()
{
    delete mInfo;
}

void FolderArchiveSettingPage::slotEnableChanged(bool enabled)
{
    mArchiveFolder->setEnabled(enabled);
    mArchiveNamed->setEnabled(enabled);
}

void FolderArchiveSettingPage::loadSettings()
{
    KConfig config(FolderArchive::FolderArchiveUtil::configFileName());
    const QString groupName = FolderArchive::FolderArchiveUtil::groupConfigPattern() + mInstanceName;
    if (config.hasGroup(groupName)) {
        KConfigGroup grp = config.group(groupName);
        mInfo = new FolderArchiveAccountInfo(grp);
        mEnabled->setChecked(mInfo->enabled());
        mArchiveFolder->setCollection(Akonadi::Collection(mInfo->archiveTopLevel()));
        mArchiveNamed->setType(mInfo->folderArchiveType());
    } else {
        mInfo = new FolderArchiveAccountInfo();
        mEnabled->setChecked(false);
    }
    slotEnableChanged(mEnabled->isChecked());
}

void FolderArchiveSettingPage::writeSettings()
{
    KConfig config(FolderArchive::FolderArchiveUtil::configFileName());
    KConfigGroup grp = config.group(FolderArchive::FolderArchiveUtil::groupConfigPattern() + mInstanceName);
    mInfo->setInstanceName(mInstanceName);
    if (mArchiveFolder->collection().isValid()) {
        mInfo->setEnabled(mEnabled->isChecked());
        mInfo->setArchiveTopLevel(mArchiveFolder->collection().id());
    } else {
        mInfo->setEnabled(false);
        mInfo->setArchiveTopLevel(-1);
    }

    mInfo->setFolderArchiveType(mArchiveNamed->type());
    mInfo->writeConfig(grp);

    //Update cache from KMail
    const QString kmailInterface = QStringLiteral("org.kde.kmail");
    QDBusReply<bool> reply = QDBusConnection::sessionBus().interface()->isServiceRegistered(kmailInterface);
    if (!reply.isValid() || !reply.value()) {
        return;
    }
    QDBusInterface kmail(kmailInterface, QStringLiteral("/KMail"), QStringLiteral("org.kde.kmail.kmail"));
    kmail.asyncCall(QStringLiteral("reloadFolderArchiveConfig"));
}
