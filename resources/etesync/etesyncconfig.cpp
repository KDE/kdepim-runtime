// SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
// SPDX-FileCopyrightText: 2025 Pablo Ariño <probpabarino@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include <Akonadi/AgentConfigurationBase>
#include <KLocalizedString>
#include <KWindowSystem>

#include "etesync_debug.h"
#include "etesyncclientstate.h"
#include "settings.h"
#include "setupwizard.h"

class EteSyncConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    EteSyncConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
        , mSettings(config)
        , mConfigClientState(std::make_unique<EteSyncClientState>(&mSettings, identifier()))
    {
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        qCDebug(ETESYNC_LOG) << "EteSyncConfig::load() called.";

        // Hide the config window
        if (const auto win = parentWidget()->window()) {
            win->hide();
        }
        QMetaObject::invokeMethod(
            this,
            [this]() {
                SetupWizard wizard(mConfigClientState.get());
                wizard.setAttribute(Qt::WA_NativeWindow, true);
                KWindowSystem::setMainWindow(wizard.windowHandle(), parentWidget()->winId());
                if (wizard.exec() == QDialog::Accepted) {
                    qCDebug(ETESYNC_LOG) << "Wizard accepted.";
                    mConfigClientState->saveSettings();
                    mConfigClientState->saveAccount();
                    if (const auto dlg = qobject_cast<QDialog *>(parentWidget()->window())) {
                        dlg->accept();
                    }
                } else {
                    qCDebug(ETESYNC_LOG) << "Wizard canceled.";
                    if (const auto dlg = qobject_cast<QDialog *>(parentWidget()->window())) {
                        dlg->rejected();
                    }
                }
            },
            Qt::QueuedConnection);
    }

    [[nodiscard]] bool save() const override
    {
        qCDebug(ETESYNC_LOG) << "EteSyncConfig::save() called. Persisting current mSettings.";
        return Akonadi::AgentConfigurationBase::save();
    }

private:
    Settings mSettings;
    std::unique_ptr<EteSyncClientState> mConfigClientState;
};

AKONADI_AGENTCONFIG_FACTORY(EteSyncConfigFactory, "etesyncconfig.json", EteSyncConfig)

#include "etesyncconfig.moc"
