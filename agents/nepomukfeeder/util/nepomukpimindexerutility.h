/*
 *   Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NEPOMUKPIMINDEXERUTILITY_H
#define NEPOMUKPIMINDEXERUTILITY_H


#include <KDE/KMainWindow>
#include <kxmlguiwindow.h>

#include "ui_nepomukpimindexerutility.h"
#include <feederqueue.h>


/**
 * This class serves as the main window for NepomukPIMindexerUtility.  It handles the
 * menus, toolbars and status bars.
 *
 * @short Main window class
 * @author Your Name <mail@example.com>
 * @version 0.1
 */
class NepomukPIMindexerUtility : public KXmlGuiWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    NepomukPIMindexerUtility();

    /**
     * Default Destructor
     */
    virtual ~NepomukPIMindexerUtility();

private slots:
    void indexCurrentlySelected();
    void reindexCurrentlySelected();
    void removeDataOfCurrentlySelected();

    void progress(int p);
    void running(const QString &p);
    void fullyIndexed();
    void removalComplete(KJob *job);

    void copyUrlFromDataCurrentlySelected();
private:
    // this is the name of the root widget inside our Ui file
    // you can rename it in designer and then change it here
    Ui::mainWidget m_ui;
    FeederQueue *mFeederQueue;
    QTime mTime;
    Akonadi::Item mRemovedItem;
};

#endif // _NEPOMUKPIMINDEXERUTILITY_H_
