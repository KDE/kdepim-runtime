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
#include "compactpage.h"
#include "lockmethodpage.h"

#include <KLocalizedString>

class MBoxConfigBase : public SingleFileResourceConfigBase<Settings>
{
public:
    MBoxConfigBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : SingleFileResourceConfigBase<Settings>(config, parent, args)
    {
        mWidget->setFilter(QStringLiteral("*.mbox|") + i18nc("Filedialog filter for *.mbox", "MBox file"));
        mWidget->addPage(i18n("Compact frequency"), new CompactPage(mSettings->path()));
        mWidget->addPage(i18n("Lock method"), new LockMethodPage());
    }
};

class MBoxConfig : public MBoxConfigBase
{
    Q_OBJECT
public:
    using MBoxConfigBase::MBoxConfigBase;
};

AKONADI_AGENTCONFIG_FACTORY(MBoxConfigFactory, "mboxconfig.json", MBoxConfig)

#include "mboxconfig.moc"
