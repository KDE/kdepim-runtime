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

#ifndef DATAMANAGEMENT_H
#define DATAMANAGEMENT_H

#include <QtCore/QObject>

#include <KComponentData>
#include <KGlobal>

#include <Soprano/Global>

#include "nepomukdatamanagement_export.h"

class KJob;
class KUrl;


// FIXME: remove this section in KDE 4.8
/**
 * \mainpage %Nepomuk Data Managment API Documentation
 *
 * This API is subject to change!
 *
 * See \ref nepomuk_datamanagement for all the gory details.
 */


namespace Nepomuk {
    class DescribeResourcesJob;
    class StoreResourcesJob;
    class CreateResourceJob;
    class SimpleResourceGraph;

    /**
     * \defgroup nepomuk_datamanagement Nepomuk Data Management
     *
     * \brief The basic Nepomuk data manipulation interface.
     *
     * The Data Management API is the basic interface to manipulate data in Nepomuk. It provides methods to add, remove,
     * change properties, add and remove resources, merge resources, integrate blobs of information, and retrieve resources
     * according to certain criteria.
     *
     * The data management API can be split into two groups: the basic and the advanced API. The basic API allows to modify
     * properties and resources in a very straight-forward manner: add or set a property value, remove property values or
     * whole properties, and so on. The advanced API on the other hand deals with more complicated situations mostly based on
     * the fact that Nepomuk always remembers which application did what. Each of the methods has a parameter which states
     * the calling component. This allows clients to for example only remove information that it actually created, making
     * updates very easy.
     *
     *
     * \section nepomuk_dms_resource_uris Resource URIs
     *
     * Most methods take a single or a list of resource URIs. Normally all resources in Nepomuk have a unique URI of the form
     * \c nepomuk:/res/UUID where \a UUID is a randomly generated universal identifer. However, the data management service
     * methods can handle a bit more than that. Any local file URL can be used where a resource URI is required. It will
     * automatically be converted to the appropriate resource URI. The same is true for any URL with a protocol supported
     * by KIO. These URLs will not be used as resource URIs but as values of the \c nie:url property.
     *
     * Thus, when setting a property on a local file URL like <tt>file:///home/user/file.txt</tt> the property will actually be set
     * on the resource corresponding to the local file. This resources has a resource URI of the form detailed above. Its
     * \c nie:url property, however, will be set to <tt>file:///home/user/file.txt</tt>.
     *
     * In addition to the above URL conversion local file paths are also supported. They will be converted to local file URLs
     * and then treated the same way as explained above.
     *
     * In general one should never create resource URIs manually. Instead use createResource() or storeResources() with an
     * empty URI in the SimpleResource. The only exception are resources that have counterparts with URLs on the desktop like
     * local files or web pages or the like.
     *
     *
     * \section nepomuk_dms_sub_resources Sub-Resources
     *
     * Nepomuk has the concept of sub-resources. The basic idea of a sub-resource is that while it is a resource in itself, it
     * does not make sense without its parent resource. The typical example would be contact details:
     *
     * \code
     * <nepomuk:/res/A> a nco:Contact ;
     *        nco:fullname "Nepomuk Nasenbär" ;
     *        nco:hasPostalAddress <nepomuk:/res/B> ;
     *        nao:hasSubResource <nepomuk:/res/B> .
     *
     * <nepomuk:/res/B> a nco:PostalAddress ;
     *        nco:streetAddress "Nepomuk street 1" ;
     *        [...] .
     * \endcode
     *
     * While in theory a \c nco:PostalAddress resource could live on its own it does not make much sense without the contact.
     * Thus, it is marked as being the sub-resource of the \c nco:Contact.
     *
     * Less obvious examples are contacts that are just created for indexing email senders or for indexing music files. These
     * are contacts the user most likely does not have need for without the original data - the email or the music file. Thus,
     * these contacts would also be marked as sub-resources of the email or music file.
     *
     * This allows for nice cleanup when removing resources. The methods removeResources() and removeDataByApplication() allow
     * to specify additional flags. One of these flags is RemoveSubResources. Specifying the flag results in the removal of
     * sub-resources.
     *
     *
     * \section nepomuk_dms_metadata Resource Metadata
     *
     * When thinking in Nepomuk terms all the information that is added is only data, not meta-data. (The file indexer which
     * reads file meta-data to store it in Nepomuk also creates just data.) However, Nepomuk also maintains its own meta-data.
     * For each resource the following properties are kept as meta-data:
     *
     * - \c nao:created - The creation date of the resource.
     * - \c nao:lastModified - The last time the resource was modified. This includes the addition, removal, the change of
     * properties, both with the resource as subject and as object.
     * - \c nao:userVisible - A boolean property stating whether the resource should be shown to the user or not. This mostly
     * applies to graphical user interfaces and is used automatically in the Nepomuk Query API.
     *
     * This information is updated automatically and cannot be changed through the API (except for special cases used for
     * syncing). But it can be queried at any time to be used for whatever purpose.
     *
     *
     * \section nepomuk_dms_resource_identification Resource Identification
     *
     * Resource identification is an important issue in storeResources(). There are basically three ways to identify a resource:
     * -# The trivial way to identify a resource is to provide the exact resource URI.
     * -# The second, also rather trivial way to identify a resource is through its nie:url. This can be a local file URL or an
     * http URL or anything else as decribed in \ref nepomuk_dms_resource_uris.
     * -# The last, most interesting way to identify a resource is through its properties and relations. This is what
     * storeResources() does in Nepomuk::IdentifyNew mode.
     *
     * In general all properties with a literal range are considered identifying. This includes properties like nao:prefLabel,
     * nie:title, nco:fullname, and so on. All properties with a non-literal range are considered non-identifying. However,
     * there are exceptions to this rule. Some properties with literal ranges are non-identifying since they express the state
     * of a resource or an opinion of a particular user.
     *
     * Examples of properties like this include \c nao:numericRating, nie:comment, or nco:imStatus. On the other hand there are
     * properties with non-literal ranges which are in fact identifying. Typical examples include \c rdf:type, \c nfo:hasHash,
     * or \c nmm:performer.
     *
     * To this end we make use of \c nrl:IdentifyingProperty and \c nrl:FluxProperty. The former is used to mark specific properties
     * as being identifying while the latter states that a property can change over time without actually chaning the identity
     * of the resource.
     *
     * In storeResources() two resources are considered being equal if all of their identifying properties match and they have at
     * least one identifying propery in common. Matching identifying properties here means that there is no identifying property
     * with a different value in the other resource.
     *
     *
     * \section nepomuk_dms_permissions Permissions in %Nepomuk
     *
     * FIXME: define exactly how permissions are handled. By default all is private. Questions remaining:
     * - Do we define permissions on the graph level?
     * - What is the range of the permissions? \c nao:Party?
     * - How do we define "public to all"?
     *
     *
     * \section nepomuk_dms_advanced Advanced Nepomuk Concepts
     *
     * This section described advanced concepts in Nepomuk such as the data layout used throughout the database.
     *
     * \subsection nepomuk_dms_graphs Named Graphs in Nepomuk
     *
     * Nepomuk makes heavy use of named graphs or contexts when talking in terms of Soprano. Essentially the named graphs allow
     * to group triples by adding a fourth node to a <a href="http://soprano.sourceforge.net/apidox/trunk/classSoprano_1_1Statement.html">statement</a>
     * which can then be used like any other resource. In Nepomuk this mechanism is used to categorize triples
     * and to store meta-data about them.
     *
     * Nepomuk makes the distincion between four basic types of graphs:
     * - \c nrl:Ontology - An ontology graph contains class and property definitions which are read by the ontology loader. Typically
     * these ontologies come from the <a href="http://oscaf.sf.net/">Shared-Desktop-Ontologies</a> package installed on the system.
     * - \c nrl:InstanceBase - The instance base is the "normal" graph type. All information added to Nepomuk by clients is stored in
     * instance base graphs.
     * - \c nrl:DiscardableInstanceBase - Being a sub-class of \c nrl:InstanceBase the discardable instance base also contains "normal"
     * information. The only difference is that this information can be recreated at any time. Thus, it is seen as \a discardable and
     * is, for example not taken into account in backups. Typical discardable information includes file meta-data created by the file indexer.
     * - \c nrl:GraphMetadata - The graph meta-data graphs only contains meta-data about graphs. This includes the type of a graph, its
     * creation date, and so on.
     *
     * In addition to its type the following information is stored about each graph, and thus, about each triple in the graph:
     * - \c nao:created - The creation date of the graph (and the triples within)
     * - \c nao:maintainedBy - The application which triggered the creation of the graph. This is important for methods like removeDataByApplication().
     *
     * Typically the information about one resource is scatterned over several graphs over time since every change to a resource leads
     * to the creation of a new graph to save the creation date and the creating application.
     *
     * The following example shows the result of indexing one local file:
     *
     * \code
     * <nepomuk:/ctx/b17ee4b5-ab8b-4fc5-bcc2-4bc25859cfa6> {
     *   <nepomuk:/res/e02fe67e-7b69-4ea7-847e-ebe2d3623d5c>
     *     nao:created “2011-05-20T11:23:45Z”^^xsd:dateTime ;
     *     nao:lastModified “2011-05-20T11:23:45Z”^^xsd:dateTime ;
     *
     *     nie:contentSize "1286"^^xsd:int ;
     *     nie:isPartOf <nepomuk:/res/80b4187c-9c40-4e98-9322-9ebcc10bd0bd> ;
     *     nie:lastModified "2010-12-14T14:49:49Z"^^xsd:dateTime ;
     *     nie:mimeType "text/plain"^^xsd:string ;
     *     nie:plainTextContent "[...]"^^xsd:string ;
     *     nie:url <file:///home/nepomuk/helloworld.txt> ;
     *     nfo:characterCount "1249"^^xsd:int ;
     *     nfo:fileName "helloworld.txt"^^xsd:string ;
     *     nfo:lineCount "37"^^xsd:int ;
     *     nfo:wordCount "126"^^xsd:int ;
     *     a nfo:PlainTextDocument, nfo:FileDataObject .
     * }
     * <nepomuk:/ctx/5cf7070e-e4f4-4a0f-8b9a-fe9d94187d82> {
     *   <nepomuk:/ctx/5cf7070e-e4f4-4a0f-8b9a-fe9d94187d82>
     *     a nrl:GraphMetadata ;
     *     nrl:coreGraphMetadataFor <nepomuk:/ctx/b17ee4b5-ab8b-4fc5-bcc2-4bc25859cfa6> .
     *
     *   <nepomuk:/ctx/b17ee4b5-ab8b-4fc5-bcc2-4bc25859cfa6>
     *     a nrl:DiscardableInstanceBase ;
     *     nao:created "2011-05-04T09:46:11.724Z"^^xsd:dateTime ;
     *     nao:maintainedBy <nepomuk:/res/someapp> .
     * }
     * \endcode
     *
     * Here one important thing is to be noted: the example contains two different last modification dates: \c nao:lastModified and \c nie:lastModified.
     * \c nie:lastModified refers to the file on disk while \c nao:lastModified refers to the Nepomuk resource in the database.
     *
     * \author Sebastian Trueg <trueg@kde.org>, Vishesh Handa <handa.vish@gmail.com>
     */
    //@{
    /**
     * \name Basic Data Managment API
     */
    //@{
    /**
     * \brief Flags to influence the behaviour of the data management methods
     * removeResources() and removeDataByApplication().
     */
    enum RemovalFlag {
        /// No flags - default behaviour
        NoRemovalFlags = 0,

        // trueg: why don't we make the removal of sub-resources the default?
        /**
         * Remove sub resources of the resources specified in the parameters.
         * This will remove sub-resources that are not referenced by any resource
         * that will not be deleted.
         * See \ref nepomuk_dms_sub_resources for details.
         */
        RemoveSubResoures = 1
    };
    Q_DECLARE_FLAGS(RemovalFlags, RemovalFlag)

    /**
     * \brief Add one or more property values to one or more resources.
     *
     * Adds property values in addition to the existing ones.
     *
     * \param resources The resources to add the new property values to. See \ref nepomuk_dms_resource_uris for details.
     * \param property The property to be changed. This needs to be the URI of a known property. If the property has
     * cardinality restrictions which would be violated by this operation it will fail.
     * \param values The values to add. For each resource and each value a triple will be created.
     * \param component The calling component. Typically this is left to the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* addProperty(const QList<QUrl>& resources,
                                                     const QUrl& property,
                                                     const QVariantList& values,
                                                     const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Set the values of a property for one or more resources.
     *
     * Sets property values overwriting/replacing the existing ones.
     *
     * \param resources The resources to add the new property values to. See \ref nepomuk_dms_resource_uris for details.
     * \param property The property to be set. This needs to be the URI of a known property. If the property has
     * cardinality restrictions which would be violated by this operation it will fail.
     * \param values The values to set. For each resource and each value a triple will be created. Existing values will
     * be overwritten.
     * \param component The calling component. Typically this is left to the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* setProperty(const QList<QUrl>& resources,
                                                     const QUrl& property,
                                                     const QVariantList& values,
                                                     const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Remove values of a property from one or more resources.
     *
     * Removes the given property values.
     *
     * \param resources The resources to remove the property values from. See \ref nepomuk_dms_resource_uris for details.
     * \param property The property to be changed. This needs to be the URI of a known property. If the property has
     * cardinality restrictions which would be violated by this operation it will fail.
     * \param values The values to remove.
     * \param component The calling component. Typically this is left to the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeProperty(const QList<QUrl>& resources,
                                                        const QUrl& property,
                                                        const QVariantList& values,
                                                        const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Remove one or more properties from one or more resources.
     *
     * Removes all values from all given properties from all given resources.
     *
     * \param resources The resources to remove the properties from. See \ref nepomuk_dms_resource_uris for details.
     * \param properties The properties to be changed. These need to be the URIs of known properties. If one pf the
     * properties has cardinality restrictions which would be violated by this operation it will fail.
     * \param component The calling component. Typically this is left to the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeProperties(const QList<QUrl>& resources,
                                                          const QList<QUrl>& properties,
                                                          const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Create a new resource.
     *
     * Creates a new resource with the given types, label, and description and returns the new resource's URI.
     *
     * \param types A list of RDF types that the new resource should have. These need to be the URIs of known RDF classes.
     * \param label The optional nao:prefLabel to be set.
     * \param description The optional nao:description to be set.
     * \param component The calling component. Typically this is left to the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT CreateResourceJob* createResource(const QList<QUrl>& types,
                                                                     const QString& label,
                                                                     const QString& description,
                                                                     const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Completely remove resources from the database.
     *
     * \param resources The resources to remove. See \ref nepomuk_dms_resource_uris for details.
     * \param flags Optional flags to change the detail of what should be removed.
     * \param component The calling component. Typically this is left to the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeResources(const QList<QUrl>& resources,
                                                         Nepomuk::RemovalFlags flags = Nepomuk::NoRemovalFlags,
                                                         const KComponentData& component = KGlobal::mainComponent());
    //@}

    /**
     * \name Advanced Data Managment API
     */
    //@{
    /**
     * \brief The identification mode used by storeResources().
     *
     * This states which given resources should be merged
     * with existing ones that match.
     */
    enum StoreIdentificationMode {
        /// This is the default mode. Only new resources without a resource URI are identified. All others
        /// are just saved with their given URI, provided the URI already exists.
        IdentifyNew = 0,

        /// All resources are treated as new ones. The only exception are those with a defined
        /// resource URI.
        IdentifyNone = 2
    };

    /**
     * \brief Flags to influence the behaviour of storeResources().
     */
    enum StoreResourcesFlag {
        /// No flags - default behaviour
        NoStoreResourcesFlags = 0,

        /// By default storeResources() will only append data and fail if properties with
        /// cardinality 1 already have a value. This flag changes the behaviour to force the
        /// new values instead.
        OverwriteProperties = 1,

        /// When lazy cardinalities are enabled any value that would violate a cardinality restriction
        /// is simply dropped without throwing an error.
        LazyCardinalities = 2
    };
    Q_DECLARE_FLAGS(StoreResourcesFlags, StoreResourcesFlag)

    /**
     * \brief Flags to influence the result of describeResources().
     *
     * See the documentation of describeResources() for details.
     */
    enum DescribeResourcesFlag {
        /// No flags - default behaviour
        NoDescribeResourcesFlags = 0,

        /// Exclude discardable data, ie. data which can be re-generated
        ExcludeDiscardableData = 1,

        /// Exclude related resources, only include literal properties
        ExcludeRelatedResources = 2
    };
    Q_DECLARE_FLAGS(DescribeResourcesFlags, DescribeResourcesFlag)

    /**
     * \brief Remove all information about resources from the database which
     * has been created by a specific application.
     *
     * \param resources The resources to remove the data from. See \ref nepomuk_dms_resource_uris for details.
     * \param flags Optional flags to change the detail of what should be removed. When specifying RemoveSubResources
     * even sub-resources created by other applications are removed if they are not referenced by other resources
     * anymore.
     * \param component The calling component. Only data created by this component is removed. Everything else
     * is left untouched. Essential properties like \a nie:url are only removed if the entire resource is
     * removed.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeDataByApplication(const QList<QUrl>& resources,
                                                                 Nepomuk::RemovalFlags flags = Nepomuk::NoRemovalFlags,
                                                                 const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Remove all information created by a specific application.
     *
     * \param flags Optional flags to change the detail of what should be removed. Here RemoveSubResources is a bit
     * special as it only applies to resources that are removed completely and spans sub-resources created by
     * other applications.
     * \param component The calling component. Only data created by this component is removed. Everything else
     * is left untouched. Essential properties like \a nie:url are only removed if the entire resource is
     * removed.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeDataByApplication(Nepomuk::RemovalFlags flags = Nepomuk::NoRemovalFlags,
                                                                 const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Merge two resources into one.
     *
     * \param resource1 The first resource to merge. If both resources have conflicting properties like different
     * values on a property with a cardinality restriction the values from resource1 take precedence.
     * See \ref nepomuk_dms_resource_uris for details.
     * \param resource2 The resource to be merged into the first. See \ref nepomuk_dms_resource_uris for details.
     * \param component The calling component. Typically this is left to the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* mergeResources(const QUrl& resource1,
                                                        const QUrl& resource2,
                                                        const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Store many resources at once.
     *
     * This is the most powerful method of them all. It allows to store a whole set of resources in one
     * go including creating new resources.
     *
     * \param resources The resources to be merged. Blank nodes (URIs of the form \a _:xyz) will be converted into new
     * URIs (unless the identificationMode allows to merge with an existing resource). See \ref nepomuk_dms_resource_uris for details.
     * \param identificationMode This method can try hard to avoid duplicate resources by looking
     * for already existing duplicates based on nrl:IdentifyingProperty. By default it only looks
     * for duplicates of resources that do not have a resource URI (SimpleResource::uri()) defined.
     * This behaviour can be changed with this parameter.
     * \param flags Additional flags to change the behaviour of the method.
     * \param additionalMetadata Additional metadata for the added resources. This can include
     * such details as the creator of the data or details on the method of data recovery.
     * One typical usecase is that the file indexer uses (rdf:type, nrl:DiscardableInstanceBase)
     * to state that the provided information can be recreated at any time. Only built-in types
     * such as int, string, or url are supported.
     * \param component The calling component. Typically this is left to the default.
     *
     * See \ref nepomuk_dms_resource_identification for details on how resources are identified.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT StoreResourcesJob* storeResources(const Nepomuk::SimpleResourceGraph& resources,
                                                        Nepomuk::StoreIdentificationMode identificationMode = Nepomuk::IdentifyNew,
                                                        Nepomuk::StoreResourcesFlags flags = Nepomuk::NoStoreResourcesFlags,
                                                        const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>(),
                                                        const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Import an RDF graph from a URL.
     *
     * This is essentially the same method as storeResources() except that it uses a different method
     * encoding the resources.
     *
     * \param url The url from which the graph should be loaded. This does not have to be local.
     * \param serialization The RDF serialization used for the file. If Soprano::SerializationUnknown a crude automatic
     * detection based on file extension is used.
     * \param userSerialization If \p serialization is Soprano::SerializationUser this value is used. See Soprano::Parser
     * for details.
     * \param identificationMode This method can try hard to avoid duplicate resources by looking
     * for already existing duplicates based on nrl:IdentifyingProperty. By default it only looks
     * for duplicates of resources that do not have a resource URI (SimpleResource::uri()) defined.
     * This behaviour can be changed with this parameter.
     * \param flags Additional flags to change the behaviour of the method.
     * \param additionalMetadata Additional metadata for the added resources. This can include
     * such details as the creator of the data or details on the method of data recovery.
     * One typical usecase is that the file indexer uses (rdf:type, nrl:DiscardableInstanceBase)
     * to state that the provided information can be recreated at any time. Only built-in types
     * such as int, string, or url are supported.
     * \param component The calling component. Typically this is left to the default.
     *
     * See \ref nepomuk_dms_resource_identification for details on how resources are identified.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* importResources(const KUrl& url,
                                                         Soprano::RdfSerialization serialization,
                                                         const QString& userSerialization = QString(),
                                                         StoreIdentificationMode identificationMode = IdentifyNew,
                                                         StoreResourcesFlags flags = NoStoreResourcesFlags,
                                                         const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>(),
                                                         const KComponentData& component = KGlobal::mainComponent());

    /**
     * \brief Retrieve all information about a set of resources.
     *
     * Different levels of detail are available when retrieving resources. These are modified through the
     * \p flags where the following values are supported:
     *
     * - \c ExcludeDiscardableData - If this flag is enabled no discardable data will be returned. This means
     * that any data that has been created through storeResources() using additional metadata including a graph
     * type \c nrl:DiscardableInstanceBase is ignored. This includes for example all information the file
     * indexer has created. Be aware that this might even mean that some of the requested \p resources are not
     * returned at all since they only contain discardable information.
     *
     * - \c ExcludeRelatedResources - If this flag is enabled related resources are ignored, only properties with
     * a literal value will be returned. The only exception are sub-resources which are treated as part of the
     * resource itself. Typical examples of sub-resources are address details of a contact or the performer
     * contact of a music track.
     *
     * \b Related \b resources:
     *
     * If the \c ExcludeRelatedResources flag is not specified related resources are returned as well. Related
     * resoures are returned by including only their identifying properties. \ref nepomuk_dms_resource_identification
     * explains the usage of identifying properties in more detail.
     *
     * \param resources The resources to describe. See \ref nepomuk_dms_resource_uris for details.
     * \param flags Optional flags to modify the data which is returned.
     * \param targetParties This optional list can be used to specify the parties (nao:Party) which should
     * receive the returned data. This will result in a filtering of the result according to configured
     * permissions. Only data which is set as being public or readable by the specified parties is returned.
     * See \ref nepomuk_dms_permissions for details. \b NOT \b IMPLEMENTED \b YET!
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT DescribeResourcesJob* describeResources(const QList<QUrl>& resources,
                                                                           DescribeResourcesFlags flags = NoDescribeResourcesFlags,
                                                                           const QList<QUrl>& targetParties = QList<QUrl>() );
    //@}
    //@}
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::RemovalFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::StoreResourcesFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::DescribeResourcesFlags)

#endif
