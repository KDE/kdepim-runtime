/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "datamanagement.h"
#include "genericdatamanagementjob_p.h"
#include "createresourcejob.h"
#include "describeresourcesjob.h"
#include "storeresourcesjob.h"
#include "dbustypes.h"
#include "simpleresourcegraph.h"

#include <QtCore/QStringList>
#include <QtCore/QMutableListIterator>

#include <KUrl>


KJob* Nepomuk::addProperty(const QList<QUrl>& resources,
                           const QUrl& property,
                           const QVariantList& values,
                           const KComponentData& component)
{
    return new GenericDataManagementJob("addProperty",
                                        Q_ARG(QStringList, Nepomuk::DBus::convertUriList(resources)),
                                        Q_ARG(QString, DBus::convertUri(property)),
                                        Q_ARG(QVariantList, Nepomuk::DBus::normalizeVariantList(values)),
                                        Q_ARG(QString, component.componentName()));
}

KJob* Nepomuk::setProperty(const QList<QUrl>& resources,
                           const QUrl& property,
                           const QVariantList& values,
                           const KComponentData& component)
{
    return new GenericDataManagementJob("setProperty",
                                        Q_ARG(QStringList, Nepomuk::DBus::convertUriList(resources)),
                                        Q_ARG(QString, DBus::convertUri(property)),
                                        Q_ARG(QVariantList, Nepomuk::DBus::normalizeVariantList(values)),
                                        Q_ARG(QString, component.componentName()));
}


KJob* Nepomuk::removeProperty(const QList<QUrl>& resources,
                              const QUrl& property,
                              const QVariantList& values,
                              const KComponentData& component)
{
    return new GenericDataManagementJob("removeProperty",
                                        Q_ARG(QStringList, Nepomuk::DBus::convertUriList(resources)),
                                        Q_ARG(QString, DBus::convertUri(property)),
                                        Q_ARG(QVariantList, Nepomuk::DBus::normalizeVariantList(values)),
                                        Q_ARG(QString, component.componentName()));
}


KJob* Nepomuk::removeProperties(const QList<QUrl>& resources,
                                const QList<QUrl>& properties,
                                const KComponentData& component)
{
    return new GenericDataManagementJob("removeProperties",
                                        Q_ARG(QStringList, Nepomuk::DBus::convertUriList(resources)),
                                        Q_ARG(QStringList, Nepomuk::DBus::convertUriList(properties)),
                                        Q_ARG(QString, component.componentName()));
}


Nepomuk::CreateResourceJob* Nepomuk::createResource(const QList<QUrl>& types,
                                                    const QString& label,
                                                    const QString& description,
                                                    const KComponentData& component)
{
    return new CreateResourceJob(types, label, description, component);
}


KJob* Nepomuk::removeResources(const QList<QUrl>& resources,
                               RemovalFlags flags,
                               const KComponentData& component)
{
    return new GenericDataManagementJob("removeResources",
                                        Q_ARG(QStringList, Nepomuk::DBus::convertUriList(resources)),
                                        Q_ARG(int, int(flags)),
                                        Q_ARG(QString, component.componentName()));
}


KJob* Nepomuk::removeDataByApplication(const QList<QUrl>& resources,
                                       RemovalFlags flags,
                                       const KComponentData& component)
{
    return new GenericDataManagementJob("removeDataByApplication",
                                        Q_ARG(QStringList, Nepomuk::DBus::convertUriList(resources)),
                                        Q_ARG(int, int(flags)),
                                        Q_ARG(QString, component.componentName()));
}


KJob* Nepomuk::removeDataByApplication(RemovalFlags flags,
                                       const KComponentData& component)
{
    return new GenericDataManagementJob("removeDataByApplication",
                                        Q_ARG(int, int(flags)),
                                        Q_ARG(QString, component.componentName()));
}


KJob* Nepomuk::mergeResources(const QUrl& resource1,
                              const QUrl& resource2,
                              const KComponentData& component)
{
    return new GenericDataManagementJob("mergeResources",
                                        Q_ARG(QString, Nepomuk::DBus::convertUri(resource1)),
                                        Q_ARG(QString, Nepomuk::DBus::convertUri(resource2)),
                                        Q_ARG(QString, component.componentName()));
}


Nepomuk::StoreResourcesJob* Nepomuk::storeResources(const Nepomuk::SimpleResourceGraph& resources,
                                                    Nepomuk::StoreIdentificationMode identificationMode,
                                                    Nepomuk::StoreResourcesFlags flags,
                                                    const QHash<QUrl, QVariant>& additionalMetadata,
                                                    const KComponentData& component)
{
    return new StoreResourcesJob( resources, identificationMode, flags,
                                  additionalMetadata, component );
}

KJob* Nepomuk::importResources(const KUrl& url,
                               Soprano::RdfSerialization serialization,
                               const QString& userSerialization,
                               StoreIdentificationMode identificationMode,
                               StoreResourcesFlags flags,
                               const QHash<QUrl, QVariant>& additionalMetadata,
                               const KComponentData& component)
{
    return new GenericDataManagementJob("importResources",
                                        Q_ARG(QString, Nepomuk::DBus::convertUri(url)),
                                        Q_ARG(QString, Soprano::serializationMimeType(serialization, userSerialization)),
                                        Q_ARG(int, int(identificationMode)),
                                        Q_ARG(int, int(flags)),
                                        Q_ARG(Nepomuk::PropertyHash, additionalMetadata),
                                        Q_ARG(QString, component.componentName()));
}

Nepomuk::DescribeResourcesJob* Nepomuk::describeResources(const QList<QUrl>& resources,
                                                          DescribeResourcesFlags flags,
                                                          const QList<QUrl>& targetParties )
{
    return new DescribeResourcesJob(resources, flags, targetParties);
}
