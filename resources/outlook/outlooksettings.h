#pragma once

#include "settingsbase.h"

class OutlookSettings : public SettingsBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Outlook.Settings")
public:
    explicit OutlookSettings(const KSharedConfigPtr &config)
    ~OutlookSettings() override = default;
};