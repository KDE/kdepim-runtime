/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davcollectionmodifyjob.h"
#include "davmanager.h"
#include "davutils.h"

#include <kio/davjob.h>
#include <kio/job.h>
#include <KLocalizedString>

DavCollectionModifyJob::DavCollectionModifyJob(const DavUtils::DavUrl &url, QObject *parent)
    : KJob(parent), mUrl(url)
{
}

void DavCollectionModifyJob::setProperty(const QString &prop, const QString &value, const QString &ns)
{
    QDomElement propElement;

    if (ns.isEmpty()) {
        propElement = mQuery.createElement(prop);
    } else {
        propElement = mQuery.createElementNS(prop, ns);
    }

    const QDomText textElement = mQuery.createTextNode(value);
    propElement.appendChild(textElement);

    mSetProperties << propElement;
}

void DavCollectionModifyJob::removeProperty(const QString &prop, const QString &ns)
{
    QDomElement propElement;

    if (ns.isEmpty()) {
        propElement = mQuery.createElement(prop);
    } else {
        propElement = mQuery.createElementNS(prop, ns);
    }

    mRemoveProperties << propElement;
}

void DavCollectionModifyJob::start()
{
    if (mSetProperties.isEmpty() && mRemoveProperties.isEmpty()) {
        setError(UserDefinedError);   // no special meaning, for now at least
        setErrorText(i18n("No properties to change or remove"));
        emitResult();
        return;
    }

    QDomDocument mQuery;
    QDomElement propertyUpdateElement = mQuery.createElementNS(QStringLiteral("DAV:"), QStringLiteral("propertyupdate"));
    mQuery.appendChild(propertyUpdateElement);

    if (!mSetProperties.isEmpty()) {
        QDomElement setElement = mQuery.createElementNS(QStringLiteral("DAV:"), QStringLiteral("set"));
        propertyUpdateElement.appendChild(setElement);

        QDomElement propElement = mQuery.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        setElement.appendChild(propElement);

        foreach (const QDomElement &element, mSetProperties) {
            propElement.appendChild(element);
        }
    }

    if (!mRemoveProperties.isEmpty()) {
        QDomElement removeElement = mQuery.createElementNS(QStringLiteral("DAV:"), QStringLiteral("remove"));
        propertyUpdateElement.appendChild(removeElement);

        QDomElement propElement = mQuery.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        removeElement.appendChild(propElement);

        foreach (const QDomElement &element, mSetProperties) {
            propElement.appendChild(element);
        }
    }

    KIO::DavJob *job = DavManager::self()->createPropPatchJob(mUrl.url(), mQuery);
    job->addMetaData(QStringLiteral("PropagateHttpHeader"), QStringLiteral("true"));
    connect(job, &KIO::DavJob::result, this, &DavCollectionModifyJob::davJobFinished);
}

void DavCollectionModifyJob::davJobFinished(KJob *job)
{
    KIO::DavJob *davJob = qobject_cast<KIO::DavJob *>(job);
    const int responseCode = davJob->queryMetaData(QLatin1String("responsecode")).isEmpty() ?
                             0 :
                             davJob->queryMetaData(QLatin1String("responsecode")).toInt();

    // KIO::DavJob does not set error() even if the HTTP status code is a 4xx or a 5xx
    if (davJob->error() || (responseCode >= 400 && responseCode < 600)) {
        QString err;
        if (davJob->error() && davJob->error() != KIO::ERR_SLAVE_DEFINED) {
            err = KIO::buildErrorString(davJob->error(), davJob->errorText());
        } else {
            err = davJob->errorText();
        }

        setError(UserDefinedError + responseCode);
        setErrorText(i18n("There was a problem with the request. The item has not been modified on the server.\n"
                          "%1 (%2).", err, responseCode));

        emitResult();
        return;
    }

    const QDomDocument response = davJob->response();
    QDomElement responseElement = DavUtils::firstChildElementNS(response.documentElement(), QStringLiteral("DAV:"), QStringLiteral("response"));

    bool hasError = false;
    QString errorText;

    // parse all propstats answers to get the eventual errors
    const QDomNodeList propstats = responseElement.elementsByTagNameNS(QLatin1String("DAV:"), QStringLiteral("propstat"));
    for (int i = 0; i < propstats.length(); ++i) {
        const QDomElement propstatElement = propstats.item(i).toElement();
        const QDomElement statusElement = DavUtils::firstChildElementNS(propstatElement, QStringLiteral("DAV:"), QStringLiteral("status"));

        const QString statusText = statusElement.text();
        if (statusText.contains(QLatin1String("200"))) {
            // Nothing special to do here, this indicates the success of the whole request
            break;
        } else {
            // Generic error
            hasError = true;
            errorText = i18n("There was an error when modifying the properties");
        }
    }

    if (hasError) {
        // Trying to get more information about the error
        const QDomElement responseDescriptionElement = DavUtils::firstChildElementNS(responseElement, QStringLiteral("DAV:"), QStringLiteral("responsedescription"));
        if (!responseDescriptionElement.isNull()) {
            errorText.append(i18n("\nThe server returned more information:\n"));
            errorText.append(responseDescriptionElement.text());
        }

        setError(UserDefinedError);
        setErrorText(errorText);
    }

    emitResult();
}
