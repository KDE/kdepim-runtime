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
 *
 *   Copyright (C) 2014 by Heena Mahour <heena393@gmail.com>
 */

#include "todo.h"
#include "todomodel.h"
#include "todofiltermodel.h"
#include "todoitemdelegate.h"
#include "todotreeview.h"
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QToolButton>
#include <QModelIndex>
#include <QTimer>
#include <QAction>
#include <QDateTime>
#include <QMap>
#include <QVariant>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#include <KColorScheme>
#include <KConfigDialog>
#include <KIcon>
#include <KStandardDirs>
#include <KDirWatch>
#include <KTabWidget>
#include <KToolInvocation>
#include <KProcess>
#include <KMessageBox>
#include <Plasma/Theme>

#include <akonadi/control.h>
#include <akonadi/agentinstance.h>
#include <akonadi/servermanager.h>
#include <akonadi/itemdeletejob.h>

K_EXPORT_PLASMA_APPLET(events, Todo)

static const int MAX_RETRIES = 12;
static const int WAIT_FOR_KO_MSECS = 2000;

Todo::Todo(QObject *parent, const QVariantList &args) :
    Plasma::PopupApplet(parent, args),
    m_graphicsWidget(0),
    m_view(0),
    m_delegate(0),
    m_todoFormatConfig(),
    m_colorConfigUi(),
    m_timer(0),
    m_agentManager(0),
    incidenceTypesDialog(0),
    collectionDialog(0),
    categoriesDialog(0),
    m_openEventWatcher(0),
    m_addEventWatcher(0),
    m_addTodoWatcher(0)
{
    KGlobal::locale()->insertCatalog("libkcal");
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);

    setPopupIcon("view-pim-tasks");

    Akonadi::ServerManager::start();
}

Todo::~Todo()
{
    delete m_view;
}

void Todo::init()
{
    KConfigGroup cg = config();

    disabledTypes = cg.readEntry("DisabledIncidenceTypes", QStringList());
    disabledCollections = cg.readEntry("DisabledCollections", QStringList());
    disabledCategories = cg.readEntry("DisabledCategories", QStringList());

    QString normalEventFormat = cg.readEntry("NormalEventFormat", QString("%{startDate} %{startTime} %{summary}"));
    QString todoFormat = cg.readEntry("TodoFormat", QString("%{dueDate} %{summary}"));
    QString noDueDateFormat = cg.readEntry("NoDueDateFormat", QString("%{summary}"));
    int dtFormat = cg.readEntry("DateFormat", ShortDateFormat);
    QString dtString = cg.readEntry("CustomDateFormat", QString("dd.MM.yyyy"));
    m_appletTitle = cg.readEntry("AppletTitle", i18n("Upcoming Todos"));
    m_period = cg.readEntry("Period", 365);
    m_recurringCount = cg.readEntry("RecurringCount", 0);

    m_urgency = cg.readEntry("UrgencyTime", 15);
    m_birthdayUrgency = cg.readEntry("BirthdayUrgencyTime", 14);

    m_urgentBg = QColor(cg.readEntry("UrgentColor", QString("#FF0000")));
    m_colors.insert(urgentColorPos, m_urgentBg);
    m_passedFg = QColor(cg.readEntry("PassedColor", QString("#C3C3C3")));
    m_colors.insert(passedColorPos, m_passedFg);
    m_todoBg = QColor(cg.readEntry("TodoColor", QString("#FFD235")));
    m_colors.insert(todoColorPos, m_todoBg);
    m_showFinishedTodos = cg.readEntry("ShowFinishedTodos", false);
    m_finishedTodoBg = QColor(cg.readEntry("FinishedTodoColor", QString("#6FACE0")));
    m_colors.insert(finishedTodoColorPos, m_finishedTodoBg);
    QStringList keys, values;
    keys << i18n("Birthday") << i18n("Holiday");
    values << QString("%{startDate} %{yearsSince}. %{summary}") << QString("%{startDate} %{summary} to %{endDate}");
    keys = cg.readEntry("CategoryFormatsKeys", keys);
    values = cg.readEntry("CategoryFormatsValues", values);

    for (int i = 0; i < keys.size(); ++i) {
        m_categoryFormat.insert(keys.at(i), values.at(i));
    }

    QStringList headerList;
    headerList << i18n("Todo for today") << i18n("Events of today") << QString::number(0);
    headerList << i18n("Todo for tomorrow") << i18n("Events for tomorrow") << QString::number(1);
    headerList << i18n("Next week's todo") << i18n("Events of the next 7 days") << QString::number(2);
    headerList << i18n("Todo for next 4 weeks") << i18n("Events for the next 4 weeks") << QString::number(8);
    headerList << i18n("Later todos ") << i18n("Events later than 4 weeks") << QString::number(29);
    m_headerItemsList = cg.readEntry("HeaderItems", headerList);
    m_autoGroupHeader = cg.readEntry("AutoGroupHeader", false);

    m_delegate = new TodoItemDelegate(this, normalEventFormat, todoFormat, noDueDateFormat, dtFormat, dtString);
    m_delegate->setCategoryFormats(m_categoryFormat);

    graphicsWidget();

    Plasma::ToolTipManager::self()->registerWidget(this);
    createToolTip();

    lastCheckTime = QDateTime::currentDateTime();
    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerExpired()));
    QTimer::singleShot(5000, this, SLOT(setupModel()));
    setBusy(true);
}

void Todo::setupModel()
{
    setBusy(false);
    m_agentManager = Akonadi::AgentManager::self();

    m_model = new TodoModel(this, m_urgency, m_birthdayUrgency, m_colors, m_recurringCount, m_autoGroupHeader);
    m_model->setCategoryColors(m_categoryColors);
    m_model->setHeaderItems(m_headerItemsList);
    if (Akonadi::ServerManager::isRunning()) {
        m_model->initModel();
        m_model->initMonitor();
    }

    m_filterModel = new TodoFilterModel(this);
    m_filterModel->setPeriod(m_period);
    m_filterModel->setShowFinishedTodos(m_showFinishedTodos);
    m_filterModel->setDisabledTypes(disabledTypes);
    m_filterModel->setExcludedCollections(disabledCollections);
    m_filterModel->setDisabledCategories(disabledCategories);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setSourceModel(m_model);

    m_view->setModel(m_filterModel);
    m_view->expandAll();
    connect(m_model, SIGNAL(modelNeedsExpanding()), m_view, SLOT(expandAll()));
    m_timer->start(10000);

    connect(Akonadi::ServerManager::self(), SIGNAL(started()), this, SLOT(akonadiStatusChanged()));
}

void Todo::akonadiStatusChanged()
{
    m_model->resetModel();
}

QGraphicsWidget *Todo::graphicsWidget()
{
    if (!m_graphicsWidget) {
        m_graphicsWidget = new QGraphicsWidget(this);
        m_graphicsWidget->setMinimumSize(200, 125);
        m_graphicsWidget->setPreferredSize(350, 200);
        QPalette p = palette();
        p.setColor( QPalette::Base, Qt::darkCyan);
        p.setColor( QPalette::AlternateBase, Qt::gray);
        p.setColor( QPalette::Button, Qt::gray );
        p.setColor( QPalette::Foreground, Qt::darkGray );
        p.setColor( QPalette::Text, Qt::yellow);
        proxyWidget = new QGraphicsProxyWidget();
        m_view = new TodoTreeView();
        proxyWidget->setWidget(m_view);
        proxyWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
        m_view->setItemDelegate(m_delegate);
        m_view->setPalette(p);
        m_view->viewport()->setPalette( p );
        connect(m_view, SIGNAL(tooltipUpdated(QString)),
                SLOT(slotUpdateTooltip(QString)));
        title = new Plasma::Label();
        title->setText("<qt><b><center>" + m_appletTitle + "</center></b></br></br>"+ "Todo list"+"</qt>");
        layout = new QGraphicsLinearLayout(Qt::Vertical);
        layout->addItem(title);
        layout->addItem(proxyWidget);

        m_graphicsWidget->setLayout(layout);
        registerAsDragHandle(m_graphicsWidget);
    }

    return m_graphicsWidget;
}
void Todo::slotUpdateTooltip(QString text)
{
    tooltip.setSubText(text);
    Plasma::ToolTipManager::self()->setContent(this, tooltip);
}

void Todo::createToolTip()
{
    tooltip = Plasma::ToolTipContent(i18n("Todo list"), "", KIcon("view-pim-tasks"));
    tooltip.setMainText(i18n("Browse through the tree view and explore/add the todos"));
    tooltip.setSubText(i18n("Move mouse over the todo items to know their details(start date, duration,percentage completion etc ."));
    tooltip.setAutohide(false);
}

void Todo::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    createToolTip();
    Plasma::ToolTipManager::self()->setContent(this, tooltip);
}
#include "todo.moc"
