// SPDX-FileCopyrightText: 2025 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include <Akonadi/AgentConfigurationBase>
#include <KLocalizedString>
#include <QLabel>
#include <QVBoxLayout>

class KolabConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    KolabConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
        , mWidget(parent)
    {
        auto layout = new QVBoxLayout(&mWidget);
        auto label = new QLabel(i18nc("@title",
                                      "The Kolab support in KMail was removed. Please migrate your account to "
                                      "use the standard IMAP and WebDAV module."));
        layout->addWidget(label);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
    }

    [[nodiscard]] bool save() const override
    {
        return Akonadi::AgentConfigurationBase::save();
    }

private:
    QWidget mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(KolablConfigFactory, "kolabconfig.json", KolabConfig)

#include "kolabconfig.moc"
