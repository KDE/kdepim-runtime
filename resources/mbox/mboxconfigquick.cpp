// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "mboxconfigquick.h"
#include "settings.h"
#include <KLocalizedString>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

MBoxConfigQuick::MBoxConfigQuick(const KSharedConfigPtr &config, const QString &identifier, QObject *parent)
    : SingleFileResourceConfigQuickBase(config, identifier, new Settings(config), {i18nc("@item:inlistbox", "MBox File (*.mbox)")}, parent)
    , mProcmailAvailable(!QStandardPaths::findExecutable(u"lockfile"_s).isEmpty())
    , mMuttAvailable(!QStandardPaths::findExecutable(u"mutt_dotlock"_s).isEmpty())
{
    connect(this, &SingleFileResourceConfigQuickBase::settingsChanged, this, &MBoxConfigQuick::mboxSettingsChanged);
}

int MBoxConfigQuick::compactFrequency() const
{
    return settings()->findItem(u"CompactFrequency"_s)->property().toInt();
}

void MBoxConfigQuick::setCompactFrequency(int value)
{
    value = qBound(0, value, 1);
    auto item = settings()->findItem(u"CompactFrequency"_s);
    if (!item->isImmutable() && compactFrequency() != value) {
        item->setProperty(value);
        Q_EMIT mboxSettingsChanged();
    }
}

int MBoxConfigQuick::messageCount() const
{
    return settings()->findItem(u"MessageCount"_s)->property().toInt();
}

void MBoxConfigQuick::setMessageCount(int value)
{
    value = qBound(1, value, 1000000);
    auto item = settings()->findItem(u"MessageCount"_s);
    if (!item->isImmutable() && messageCount() != value) {
        item->setProperty(value);
        Q_EMIT mboxSettingsChanged();
    }
}

int MBoxConfigQuick::lockfileMethod() const
{
    return settings()->findItem(u"LockfileMethod"_s)->property().toInt();
}

void MBoxConfigQuick::setLockfileMethod(int value)
{
    value = qBound(0, value, 3);
    auto item = settings()->findItem(u"LockfileMethod"_s);
    if (!item->isImmutable() && lockfileMethod() != value) {
        item->setProperty(value);
        Q_EMIT mboxSettingsChanged();
    }
}

QString MBoxConfigQuick::lockfile() const
{
    return settings()->findItem(u"Lockfile"_s)->property().toString();
}

void MBoxConfigQuick::setLockfile(const QString &value)
{
    auto item = settings()->findItem(u"Lockfile"_s);
    if (!item->isImmutable() && lockfile() != value) {
        item->setProperty(value);
        Q_EMIT mboxSettingsChanged();
    }
}

bool MBoxConfigQuick::procmailAvailable() const
{
    return mProcmailAvailable;
}

bool MBoxConfigQuick::muttAvailable() const
{
    return mMuttAvailable;
}

void MBoxConfigQuick::load()
{
    SingleFileResourceConfigQuickBase::load();

    const int method = lockfileMethod();
    if (method == Settings::procmail && !mProcmailAvailable) {
        setLockfileMethod(mMuttAvailable ? Settings::mutt_dotlock : Settings::none);
    } else if ((method == Settings::mutt_dotlock || method == Settings::mutt_dotlock_privileged) && !mMuttAvailable) {
        setLockfileMethod(mProcmailAvailable ? Settings::procmail : Settings::none);
    }
}

AKONADI_AGENTCONFIG_QUICK_FACTORY(MBoxConfigQuickFactory, "mboxconfigquick.json", MBoxConfigQuick, "org.kde.akonadi.mboxconfig", "MBoxConfigurationPage")

#include "mboxconfigquick.moc"
