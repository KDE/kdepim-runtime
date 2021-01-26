/*
    SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSSETTINGS_UT_MOCK_H
#define EWSSETTINGS_UT_MOCK_H

#include <QLoggingCategory>
#include <QWidget>
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
