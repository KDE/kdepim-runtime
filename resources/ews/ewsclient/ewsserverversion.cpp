/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsserverversion.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ewsclient_debug.h"
#include "ewstypes.h"

using namespace Qt::StringLiterals;

const EwsServerVersion EwsServerVersion::ewsVersion2007(8, 0, u"Exchange2007"_s, u"Exchange 2007"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp1(8, 1, u"Exchange2007_SP1"_s, u"Exchange 2007 SP1"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp2(8, 2, u"Exchange2007_SP2"_s, u"Exchange 2007 SP2"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2007Sp3(8, 3, u"Exchange2007_SP3"_s, u"Exchange 2007 SP3"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2010(14, 0, u"Exchange2010"_s, u"Exchange 2010"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp1(14, 1, u"Exchange2010_SP1"_s, u"Exchange 2010 SP1"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp2(14, 2, u"Exchange2010_SP2"_s, u"Exchange 2010 SP2"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2010Sp3(14, 3, u"Exchange2010_SP3"_s, u"Exchange 2010 SP3"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2013(15, 0, u"Exchange2013"_s, u"Exchange 2013"_s);
const EwsServerVersion EwsServerVersion::ewsVersion2016(15, 1, u"Exchange2016"_s, u"Exchange 2016"_s);

static const EwsServerVersion ewsNullVersion;

EwsServerVersion::EwsServerVersion(QXmlStreamReader &reader)
    : mMajor(0)
    , mMinor(0)
{
    // <h:ServerVersionInfo MajorVersion=\"14\" MinorVersion=\"3\" MajorBuildNumber=\"248\"
    // MinorBuildNumber=\"2\" Version=\"Exchange2010_SP2\" />
    const QXmlStreamAttributes attrs = reader.attributes();

    auto majorRef = attrs.value("MajorVersion"_L1);
    auto minorRef = attrs.value("MinorVersion"_L1);
    auto majorBuildRef = attrs.value("MajorBuildNumber"_L1);
    auto minorBuildRef = attrs.value("MinorBuildNumber"_L1);
    auto nameRef = attrs.value("Version"_L1);

    if (majorRef.isNull() || minorRef.isNull()) {
        qCWarningNC(EWSCLI_LOG) << u"Error reading server version info - missing attribute."_s;
        return;
    }

    bool ok;
    uint majorVer = majorRef.toUInt(&ok);
    if (!ok) {
        qCWarningNC(EWSCLI_LOG) << u"Error reading server version info - invalid major version number."_s;
        return;
    }
    uint minorVer = minorRef.toUInt(&ok);
    if (!ok) {
        qCWarningNC(EWSCLI_LOG) << u"Error reading server version info - invalid minor version number."_s;
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
    writer.writeStartElement(ewsTypeNsUri, "RequestServerVersion"_L1);
    writer.writeAttribute("Version"_L1, mName);
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

    QString version(u"%1.%2"_s.arg(mMajor).arg(mMinor));

    if (mMajorBuild + mMinorBuild > 0) {
        version.append(u".%1.%2"_s.arg(mMajorBuild).arg(mMinorBuild));
    }

    for (const EwsServerVersion &ver : knownVersions) {
        if (*this == ver) {
            version.append(u" ("_s + ver.mFriendlyName + u')');
        }
    }

    return version;
}

QDebug operator<<(QDebug debug, const EwsServerVersion &version)
{
    QDebugStateSaver saver(debug);
    QDebug d = debug.nospace().noquote();
    d << u"EwsServerVersion("_s;
    if (version.isValid()) {
        d << version.majorVersion() << u", "_s << version.minorVersion() << u", "_s << version.name();
    }
    d << u')';
    return debug;
}
