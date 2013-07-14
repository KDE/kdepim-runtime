#ifndef CALENDARPLUGIN_H
#define CALENDARPLUGIN_H

#include <QDeclarativeExtensionPlugin>


class CalendarPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

Q_EXPORT_PLUGIN2(calendarplugin, CalendarPlugin)

#endif