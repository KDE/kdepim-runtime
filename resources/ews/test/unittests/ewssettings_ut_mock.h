/*
    Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
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

#ifndef EWSSETTINGS_UT_MOCK_H
#define EWSSETTINGS_UT_MOCK_H

#include <QWidget>
#include <QLoggingCategory>
#include <functional>

Q_DECLARE_LOGGING_CATEGORY(EWSRES_LOG)

class EwsSettingsBase : public QObject
{
    Q_OBJECT
public:
    class Config
    {
    public:
        QString name() const
        {
            return QStringLiteral("test_resource_name");
        }
    };

    EwsSettingsBase()
    {
    }

    virtual ~EwsSettingsBase() = default;
    QString username() const
    {
        return QStringLiteral("testuser");
    }

    QString email() const
    {
        return QStringLiteral("test@example.com");
    }

    const Config *config() const
    {
        return &mConfig;
    }

    Config mConfig;
};

#endif /* EWSSETTINGSBASE_H */
