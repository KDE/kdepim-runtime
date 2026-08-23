/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Configuration plugin for the Graph resource: account name, primary email/UPN,
    Azure tenant and client id, and the delta poll interval. Loaded by the Akonadi
    agent configuration dialog in the client process (KMail etc.); the resource picks
    the new settings up via AgentBase::reloadConfiguration(). Built in code (no .ui)
    to keep it small.
*/

#include <Akonadi/AgentConfigurationBase>
#include <Akonadi/AgentManager>

#include <KConfigGroup>
#include <KLocalizedString>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

class GraphConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    GraphConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
        , mName(new QLineEdit(parent))
        , mEmail(new QLineEdit(parent))
        , mTenant(new QLineEdit(parent))
        , mClientId(new QLineEdit(parent))
        , mPollInterval(new QSpinBox(parent))
    {
        // The host (AgentConfigurationWidget) already put a QVBoxLayout on parent;
        // plugins add one container widget to it (cf. contactssettingswidget.cpp).
        auto mainWidget = new QWidget(parent);
        parent->layout()->addWidget(mainWidget);
        auto layout = new QVBoxLayout(mainWidget);
        layout->setContentsMargins({});
        auto form = new QFormLayout;
        layout->addLayout(form);

        form->addRow(i18nc("@label:textbox", "Account name:"), mName);

        mEmail->setPlaceholderText(QStringLiteral("user@example.com"));
        form->addRow(i18nc("@label:textbox", "Email / UPN:"), mEmail);

        mTenant->setPlaceholderText(QStringLiteral("common"));
        form->addRow(i18nc("@label:textbox", "Azure tenant:"), mTenant);

        form->addRow(i18nc("@label:textbox", "Application (client) ID:"), mClientId);

        mPollInterval->setRange(1, 1440);
        mPollInterval->setSuffix(i18nc("@item:valuesuffix minutes", " min"));
        form->addRow(i18nc("@label:spinbox", "Check for new mail every:"), mPollInterval);

        auto hint = new QLabel(i18nc("@info", "Changing the tenant or client id requires signing in again."), mainWidget);
        hint->setWordWrap(true);
        hint->setEnabled(false);
        layout->addWidget(hint);
        layout->addStretch();
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mName->setText(Akonadi::AgentManager::self()->instance(identifier()).name());
        const KConfigGroup group = config()->group(QStringLiteral("General"));
        mEmail->setText(group.readEntry("Email"));
        mTenant->setText(group.readEntry("TenantId", QStringLiteral("common")));
        mClientId->setText(group.readEntry("ClientId"));
        mPollInterval->setValue(group.readEntry("PollInterval", 5));
    }

    [[nodiscard]] bool save() const override
    {
        KConfigGroup group = config()->group(QStringLiteral("General"));
        group.writeEntry("Email", mEmail->text().trimmed());
        const QString tenant = mTenant->text().trimmed();
        group.writeEntry("TenantId", tenant.isEmpty() ? QStringLiteral("common") : tenant);
        group.writeEntry("ClientId", mClientId->text().trimmed());
        group.writeEntry("PollInterval", mPollInterval->value());
        config()->sync();

        const QString name = mName->text().trimmed();
        auto instance = Akonadi::AgentManager::self()->instance(identifier());
        if (!name.isEmpty() && name != instance.name()) {
            instance.setName(name);
        }
        return Akonadi::AgentConfigurationBase::save();
    }

private:
    QLineEdit *const mName;
    QLineEdit *const mEmail;
    QLineEdit *const mTenant;
    QLineEdit *const mClientId;
    QSpinBox *const mPollInterval;
};

AKONADI_AGENTCONFIG_FACTORY(GraphConfigFactory, "graphconfig.json", GraphConfig)

#include "graphconfig.moc"
