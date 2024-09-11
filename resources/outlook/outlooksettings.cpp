#include "outlooksettings.h"

OutlookSettings::OutlookSettings(const KSharedConfigPtr &config)
    : SettingsBase(config)
{
    load();
}
