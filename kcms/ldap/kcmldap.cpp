/*
  This file is part of libkldap.

  Copyright (c) 2002-2009 Tobias Koenig <tokoe@kde.org>
  Copyright (C) 2013-2020 Laurent Montel <montel@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General  Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "kcmldap_p.h"

#include <QVBoxLayout>

#include <KAboutData>
#include <KPluginFactory>
#include <KLocalizedString>
#include <KLDAP/LdapConfigureWidget>

K_PLUGIN_CLASS_WITH_JSON(KCMLdap, "kcmldap.json")

KCMLdap::KCMLdap(QWidget *parent, const QVariantList &)
    : KCModule(parent)
{
    setButtons(KCModule::Apply);
    KAboutData *about = new KAboutData(QStringLiteral("kcmldap"),
                                       i18n("kcmldap"),
                                       QString(),
                                       i18n("LDAP Server Settings"),
                                       KAboutLicense::LGPL,
                                       i18n("(c) 2009 - 2010 Tobias Koenig"));
    about->addAuthor(i18n("Tobias Koenig"), QString(), QStringLiteral("tokoe@kde.org"));
    setAboutData(about);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    mLdapConfigureWidget = new KLDAP::LdapConfigureWidget(this);
    layout->addWidget(mLdapConfigureWidget);

    connect(mLdapConfigureWidget, &KLDAP::LdapConfigureWidget::changed, this, qOverload<bool>(&KCMLdap::changed));
}

KCMLdap::~KCMLdap()
{
}

void KCMLdap::load()
{
    mLdapConfigureWidget->load();
}

void KCMLdap::save()
{
    mLdapConfigureWidget->save();
}

#include "kcmldap.moc"
