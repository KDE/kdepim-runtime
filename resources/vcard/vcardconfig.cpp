/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include "singlefileresourceconfigbase.h"
#include "settings.h"

class VCardConfigBase : public SingleFileResourceConfigBase<SETTINGS_NAMESPACE::Settings>
{
public:
    VCardConfigBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &list)
        : SingleFileResourceConfigBase(config, parent, list)
    {
        mWidget->setFilter(QStringLiteral("*.vcf|") + i18nc("Filedialog filter for *.vcf", "vCard Address Book File"));
    }

};

class VCardConfig : public VCardConfigBase
{
    Q_OBJECT
public:
    ~VCardConfig() override {}
    using VCardConfigBase::VCardConfigBase;
};

AKONADI_AGENTCONFIG_FACTORY(VCardConfigFactory, "vcardconfig.json", VCardConfig)

#include "vcardconfig.moc"
