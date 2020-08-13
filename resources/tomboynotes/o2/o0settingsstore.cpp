/*
    SPDX-License-Identifier: BSD-2-Clause
*/

#include <QCryptographicHash>

#include "o2/o0settingsstore.h"

static quint64 getHash(const QString &encryptionKey)
{
    return QCryptographicHash::hash(encryptionKey.toLatin1(), QCryptographicHash::Sha1).toULongLong();
}

O0SettingsStore::O0SettingsStore(const QString &encryptionKey, QObject *parent)
    : O0AbstractStore(parent)
    , crypt_(getHash(encryptionKey))
{
    settings_ = new QSettings(this);
}

O0SettingsStore::O0SettingsStore(QSettings *settings, const QString &encryptionKey, QObject *parent)
    : O0AbstractStore(parent)
    , crypt_(getHash(encryptionKey))
{
    settings_ = settings;
    settings_->setParent(this);
}

QString O0SettingsStore::groupKey() const
{
    return groupKey_;
}

void O0SettingsStore::setGroupKey(const QString &groupKey)
{
    if (groupKey_ == groupKey) {
        return;
    }
    groupKey_ = groupKey;
    Q_EMIT groupKeyChanged();
}

QString O0SettingsStore::value(const QString &key, const QString &defaultValue)
{
    QString fullKey = groupKey_.isEmpty() ? key : (groupKey_ + QLatin1Char('/') + key);
    if (!settings_->contains(fullKey)) {
        return defaultValue;
    }
    return crypt_.decryptToString(settings_->value(fullKey).toString());
}

void O0SettingsStore::setValue(const QString &key, const QString &value)
{
    QString fullKey = groupKey_.isEmpty() ? key : (groupKey_ + QLatin1Char('/') + key);
    settings_->setValue(fullKey, crypt_.encryptToString(value));
}
