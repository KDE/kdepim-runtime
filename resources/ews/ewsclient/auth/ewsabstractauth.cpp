/*
    Copyright (C) 2018 Krzysztof Nowicki <krissn@op.pl>

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

#include "ewsabstractauth.h"

#include <QFile>

EwsAbstractAuth::EwsAbstractAuth(QObject *parent)
    : QObject(parent), mAuthParentWidget(nullptr)
{
}

void EwsAbstractAuth::setAuthParentWidget(QWidget *widget)
{
    mAuthParentWidget = widget;
}

void EwsAbstractAuth::notifyRequestAuthFailed()
{
    Q_EMIT requestAuthFailed();
}

void EwsAbstractAuth::setPKeyAuthCertificateFiles(const QString &certFile, const QString &pkeyFile)
{
    if (QFile::exists(certFile) && QFile::exists(pkeyFile)) {
        mPKeyCertFile = certFile;
        mPKeyKeyFile = pkeyFile;
    }
}
