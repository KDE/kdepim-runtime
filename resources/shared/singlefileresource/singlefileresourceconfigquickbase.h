// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <Akonadi/AgentConfigurationQuickBase>
#include <KCoreConfigSkeleton>
#include <QTimer>
#include <QUrl>
#include <qqmlregistration.h>

class KJob;
namespace KIO
{
class StatJob;
}

class SingleFileResourceConfigQuickBase : public Akonadi::AgentConfigurationQuickBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the resource configuration plugin")
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY settingsChanged)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY settingsChanged)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY settingsChanged)
    Q_PROPERTY(bool monitorFile READ monitorFile WRITE setMonitorFile NOTIFY settingsChanged)
    Q_PROPERTY(bool periodicUpdate READ periodicUpdate WRITE setPeriodicUpdate NOTIFY settingsChanged)
    Q_PROPERTY(int updatePeriod READ updatePeriod WRITE setUpdatePeriod NOTIFY settingsChanged)
    Q_PROPERTY(bool pathEditable READ pathEditable NOTIFY settingsChanged)
    Q_PROPERTY(bool localFile READ localFile NOTIFY settingsChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool statusIsError READ statusIsError NOTIFY statusChanged)
    Q_PROPERTY(QStringList nameFilters READ nameFilters CONSTANT)
public:
    SingleFileResourceConfigQuickBase(const KSharedConfigPtr &config,
                                      const QString &identifier,
                                      KCoreConfigSkeleton *settings,
                                      const QStringList &nameFilters,
                                      QObject *parent);
    ~SingleFileResourceConfigQuickBase() override;
    QString path() const;
    void setPath(const QString &value);
    QString displayName() const;
    void setDisplayName(const QString &value);
    bool readOnly() const;
    void setReadOnly(bool value);
    bool monitorFile() const;
    void setMonitorFile(bool value);
    bool periodicUpdate() const;
    void setPeriodicUpdate(bool value);
    int updatePeriod() const;
    void setUpdatePeriod(int value);
    bool pathEditable() const;
    bool localFile() const;
    QString status() const;
    bool statusIsError() const;
    QStringList nameFilters() const;
    Q_INVOKABLE void setFileUrl(const QUrl &url);
    void load() override;
    bool save() override;
Q_SIGNALS:
    void settingsChanged();
    void statusChanged();

protected:
    KCoreConfigSkeleton *settings() const
    {
        return mSettings;
    }

private:
    [[nodiscard]] QUrl url() const;
    void validate();
    void startStat();
    void statFinished(KJob *job);
    void setValid(bool valid);
    void setRemoteMode(bool remote);
    KCoreConfigSkeleton *mSettings;
    QStringList mNameFilters;
    QString mStatus;
    QTimer mStatTimer;
    KIO::StatJob *mStatJob = nullptr;
    bool mPathEditable = true;
    bool mValid = false;
    bool mCheckingParent = false;
    bool mStatusIsError = false;
    bool mRemoteMode = false;
    bool mLocalReadOnly = false;
    bool mLocalMonitorFile = true;
};
