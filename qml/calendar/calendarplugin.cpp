#include "calendarplugin.h"

#include "calendardata.h"
#include "calendar.h"
#include "incidencemodifier.h"
#include <qdeclarative.h>
#include <QAbstractItemModel>
#include <QAbstractListModel>


void CalendarPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.pim.calendar"));
    qmlRegisterType<CalendarData>(uri, 1, 0, "CalendarData");
    qmlRegisterType<Calendar>(uri, 1, 0, "Calendar");
    qmlRegisterType<IncidenceModifier>(uri, 1, 0, "IncidenceModifier");
    qmlRegisterType<QAbstractItemModel>();
    qmlRegisterType<QAbstractListModel>();
}

// If i add this line in calendarplugin.h it gives me compile errors.. I don't know why that happens, but this seems to work around it.
Q_EXPORT_PLUGIN2(calendarplugin, CalendarPlugin)
