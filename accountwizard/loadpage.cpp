/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "loadpage.h"
#include "global.h"
#include <kassistantdialog.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kross/core/action.h>
#include <QtCore/qfile.h>
#include <KLocalizedString>
#include <KLocalizedString>

LoadPage::LoadPage(KAssistantDialog *parent) :
    Page(parent),
    m_action(0)
{
    ui.setupUi(this);
    setValid(false);
}

void LoadPage::enterPageNext()
{
    setValid(false);
    // FIXME: deletion seems to delete the exported objects as well, killing the entire wizard...
    //delete m_action;
    m_action = 0;
    emit aboutToStart();

    const KConfig f(Global::assistant());
    KConfigGroup grp(&f, "Wizard");
    const QString scriptFile = grp.readEntry("Script", QString());
    if (scriptFile.isEmpty()) {
        ui.statusLabel->setText(i18n("No script specified in '%1'.", Global::assistant()));
        return;
    }
    if (!QFile::exists(Global::assistantBasePath() + scriptFile)) {
        ui.statusLabel->setText(i18n("Unable to load assistant: File '%1' does not exist.", Global::assistantBasePath() + scriptFile));
        return;
    }
    ui.statusLabel->setText(i18n("Loading script '%1'...", Global::assistantBasePath() + scriptFile));

    m_action = new Kross::Action(this, QLatin1String("AccountWizard"));
    typedef QPair<QObject *, QString> ObjectStringPair;
    foreach (const ObjectStringPair &exportedObject, m_exportedObjects) {
        m_action->addQObject(exportedObject.first, exportedObject.second);
    }

    if (!m_action->setFile(Global::assistantBasePath() + scriptFile)) {
        ui.statusLabel->setText(i18n("Failed to load script: '%1'.", m_action->errorMessage()));
        return;
    }

    KConfigGroup grpTranslate(&f, "Translate");
    const QString poFileName = grpTranslate.readEntry("Filename");
    if (!poFileName.isEmpty())
        //QT5 KLocalizedString::global()->insertCatalog( poFileName );

    {
        m_action->trigger();
    }

    m_parent->next();
}

void LoadPage::enterPageBack()
{
    // TODO: if we are the first page, call enterPageNext(), hm, can we get here then at all?
    m_parent->back();
}

void LoadPage::exportObject(QObject *object, const QString &name)
{
    m_exportedObjects.push_back(qMakePair(object, name));
}

