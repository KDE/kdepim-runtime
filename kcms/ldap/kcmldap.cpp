/*
  This file is part of libkldap.

  SPDX-FileCopyrightText: 2002-2009 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2022 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcmldap_p.h"

#include <QVBoxLayout>

#include <KAboutData>
#include <KLDAP/LdapConfigureWidget>
#include <KLocalizedString>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(KCMLdap, "kcmldap.json")

KCMLdap::KCMLdap(QWidget *parent, const QVariantList &)
    : KCModule(parent)
    , mLdapConfigureWidget(new KLDAP::LdapConfigureWidget(this))
{
    setButtons(KCModule::Apply);
    auto about = new KAboutData(QStringLiteral("kcmldap"),
                                i18n("kcmldap"),
                                QString(),
                                i18n("LDAP Server Settings"),
                                KAboutLicense::LGPL,
                                i18n("(c) 2009 - 2010 Tobias Koenig"));
    about->addAuthor(i18n("Tobias Koenig"), QString(), QStringLiteral("tokoe@kde.org"));
    setAboutData(about);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    layout->addWidget(mLdapConfigureWidget);

    connect(mLdapConfigureWidget, &KLDAP::LdapConfigureWidget::changed, this, qOverload<bool>(&KCMLdap::changed));
}

KCMLdap::~KCMLdap() = default;

void KCMLdap::load()
{
    mLdapConfigureWidget->load();
}

void KCMLdap::save()
{
    mLdapConfigureWidget->save();
}

#include "kcmldap.moc"
