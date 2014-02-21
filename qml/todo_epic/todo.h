/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 *    Copyright (C) 2014 by Heena Mahour <heena393@gmail.com>
 */

#ifndef TODO_H
#define TODO_H
#include "ui_todoformatconfig.h"
#include "ui_eventappletcolorconfig.h"
#include <QHash>
#include <QGraphicsSceneHoverEvent>
#include <Plasma/PopupApplet>
#include <Plasma/Label>
#include <Plasma/ToolTipManager>

#include <akonadi/agentmanager.h>

class QDateTime;
class QTimer;
class QModelIndex;
class QAction;
class KConfigDialog;
class CheckBoxDialog;
class TodoModel;
class TodoFilterModel;
class TodoItemDelegate;
class QGraphicsLinearLayout;
class QGraphicsProxyWidget;
class TodoTreeView;
class KDirWatch;
class QDBusServiceWatcher;

class Todo : public Plasma::PopupApplet
{
    Q_OBJECT

public:
    Todo(QObject *parent, const QVariantList &args);
    ~Todo();

    void init();
    QGraphicsWidget *graphicsWidget();

private slots:
    void slotUpdateTooltip(QString);
    void setupModel();
    void akonadiStatusChanged();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

private:
    void createToolTip();

private:
    QGraphicsWidget *m_graphicsWidget;
    QGraphicsLinearLayout *layout;
    TodoModel *m_model;
    TodoFilterModel *m_filterModel;
    KDirWatch *m_categoryColorWatch;
    QGraphicsProxyWidget *proxyWidget;
    TodoTreeView *m_view;
    Plasma::ToolTipContent tooltip;
    Plasma::Label *title;
    TodoItemDelegate *m_delegate;
    Ui::TodoFormatConfig m_todoFormatConfig;
    Ui::EventAppletColorConfig m_colorConfigUi;
    int m_urgency, m_birthdayUrgency, m_checkInterval, m_period, m_recurringCount;
    QColor m_urgentBg, m_passedFg, m_todoBg, m_finishedTodoBg;
    QHash<QString, QColor> m_categoryColors;
    QMap<QString, QString> m_categoryFormat;
    QList<QColor> m_colors;
    QTimer *m_timer;
    Akonadi::AgentManager *m_agentManager;
    QStringList disabledTypes, disabledCollections, disabledCategories, m_headerItemsList, m_categories;
    CheckBoxDialog *incidenceTypesDialog, *collectionDialog, *categoriesDialog;
    QDateTime lastCheckTime;
    bool m_showFinishedTodos, m_autoGroupHeader;
    QString m_uid, m_appletTitle;
    QModelIndex m_indexAtCursor;
    QDBusServiceWatcher *m_openEventWatcher, *m_addEventWatcher, *m_addTodoWatcher;
};

#endif
