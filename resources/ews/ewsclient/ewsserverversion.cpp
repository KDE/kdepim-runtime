/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsserverversion.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ewsclient_debug.h"
#include "ewstypes.h"

const EwsServerVersion EwsServerVersion::ewsVersion2007(8, 0, QStringLiteral("Exchange2007"), QStringLiteral("Exchange 2007"));
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp1(8, 1, QStringLiteral("Exchange2007_SP1"), QStringLiteral("Exchange 2007 SP1"));
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp2(8, 2, QStringLiteral("Exchange2007_SP2"), QStringLiteral("Exchange 2007 SP2"));
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp3(8, 3, QStringLiteral("Exchange2007_SP3"), QStringLiteral("Exchange 2007 SP3"));
const EwsServerVersion EwsServerVersion::ewsVersion2010(14, 0, QStringLiteral("Exchange2010"), QStringLiteral("Exchange 2010"));
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp1(14, 1, QStringLiteral("Exchange2010_SP1"), QStringLiteral("Exchange 2010 SP1"));
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp2(14, 2, QStringLiteral("Exchange2010_SP2"), QStringLiteral("Exchange 2010 SP2"));
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp3(14, 3, QStringLiteral("Exchange2010_SP3"), QStringLiteral("Exchange 2010 SP3"));
const EwsServerVersion EwsServerVersion::ewsVersion2013(15, 0, QStringLiteral("Exchange2013"), QStringLiteral("Exchange 2013"));
const EwsServerVersion EwsServerVersion::ewsVersion2016(15, 1, QStringLiteral("Exchange2016"), QStringLiteral("Exchange 2016"));

static const EwsServerVersion ewsNullVersion;

EwsServerVersion::EwsServerVersion(QXmlStreamReader &reader)
    : mMajor(0)
    , mMinor(0)
{
    // <h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" MajorBuildNumber=\"248\"
    // MinorBuildNumber=\"2\" Version=\"Exchange2010_SP2\" />
    QXmlStreamAttributes attrs = reader.attributes();

    auto majorRef = attrs.value(QStringLiteral("MajorVersion"));
    auto minorRef = attrs.value(QStringLiteral("MinorVersion"));
    auto majorBuildRef = attrs.value(QStringLiteral("MajorBuildNumber"));
    auto minorBuildRef = attrs.value(QStringLiteral("MinorBuildNumber"));
    auto nameRef = attrs.value(QStringLiteral("Version"));

    if (majorRef.isNull() || minorRef.isNull()) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading server version info - missing attribute.");
        return;
    }

    bool ok;
    uint majorVer = majorRef.toUInt(&ok);
    if (!ok) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading server version info - invalid major version number.");
        return;
    }
    uint minorVer = minorRef.toUInt(&ok);
    if (!ok) {
        qCWarningNC(EWSCLI_LOG) << QStringLiteral("Error reading server version info - invalid minor version number.");
        return;
    }

    mMajor = majorVer;
    mMinor = minorVer;
    mMajorBuild = majorBuildRef.toUInt();
    mMinorBuild = minorBuildRef.toUInt();
    mName = nameRef.toString();
}

void EwsServerVersion::writeRequestServerVersion(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(ewsTypeNsUri, QStringLiteral("RequestServerVersion"));
    writer.writeAttribute(QStringLiteral("Version"), mName);
    writer.writeEndElement();
}

bool EwsServerVersion::supports(ServerFeature feature) const
{
    const EwsServerVersion &minVer = minSupporting(feature);
    if (minVer.isValid()) {
        return *this >= minVer;
    } else {
        return false;
    }
}

const EwsServerVersion &EwsServerVersion::minSupporting(ServerFeature feature)
{
    switch (feature) {
    case StreamingSubscription:
    case FreeBusyChangedEvent:
        return ewsVersion2010Sp1;
    default:
        return ewsNullVersion;
    }
}

QString EwsServerVersion::toString() const
{
    static const QList<EwsServerVersion> knownVersions = {
        ewsVersion2007,
        ewsVersion2007Sp1,
        ewsVersion2007Sp2,
        ewsVersion2007Sp3,
        ewsVersion2010,
        ewsVersion2010Sp1,
        ewsVersion2010Sp2,
        ewsVersion2010Sp3,
        ewsVersion2013,
        ewsVersion2016,
    };

    QString version(QStringLiteral("%1.%2").arg(mMajor).arg(mMinor));

    if (mMajorBuild + mMinorBuild > 0) {
        version.append(QStringLiteral(".%1.%2").arg(mMajorBuild).arg(mMinorBuild));
    }

    for (const EwsServerVersion &ver : knownVersions) {
        if (*this == ver) {
            version.append(QStringLiteral(" (") + ver.mFriendlyName + QLatin1Char(')'));
        }
    }

    return version;
}

QDebug operator<<(QDebug debug, const EwsServerVersion &version)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << QStringLiteral("EwsServerVersion(");
    if (version.isValid()) {
        d << version.majorVersion() << QStringLiteral(", ") << version.minorVersion() << QStringLiteral(", ") << version.name();
    }
    d << QStringLiteral(")");
    return debug;
}
