/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadi-singlefileresource_export.h"
#include <Akonadi/ResourceBase>

#include <QFuture>
#include <QStringList>
#include <QUrl>

namespace KIO
{
class FileCopyJob;
class Job;
}

namespace Akonadi
{
/**
 * Base class for single file based resources.
 * @see SingleFileResource
 */
class AKONADI_SINGLEFILERESOURCE_EXPORT SingleFileResourceBase : public ResourceBase, public AgentBase::Observer
{
    Q_OBJECT
public:
    explicit SingleFileResourceBase(const QString &id);

    /**
     * Set the mimetypes supported by this resource and an optional icon for the collection.
     */
    void setSupportedMimetypes(const QStringList &mimeTypes, const QString &icon = QString());

    void collectionChanged(const Akonadi::Collection &collection) override;

public Q_SLOTS:
    [[nodiscard]] QFuture<bool> reloadFile();

    /*
     * Read the current state from a file. This can happen
     * from direct callers, or as part of a scheduled task.
     * @p taskContext specifies whether the method is being
     * called from within a task or not.
     *
     * @return Returns a QFuture that will be fulfilled once the
     * file has been read. The boolean value indicates whether
     * the file was read successfully (@p true) or whenther an
     * error has occurred (@p false).
     */
    [[nodiscard]] virtual QFuture<bool> readFile(bool taskContext = false) = 0;
    /*
     * Writes the current state out to a file. This can happen
     * from direct callers, or as part of a scheduled task.
     * @p taskContext specifies whether the method is being
     * called from within a task or not.
     */
    virtual void writeFile(bool taskContext = false) = 0;

    /*
     * Same method as above, but uses a QVariant so it can
     * be called from Akonadi::ResourceScheduler.
     */
    virtual void writeFile(const QVariant &taskContext) = 0;

protected:
    /**
     * Implement in derived classes to do things when the configuration changes
     * before reload file is called.
     */
    virtual void applyConfigurationChanges();

    /**
     * Returns a pointer to the KConfig object which is used to store runtime
     * information of the resource.
     */
    KSharedConfig::Ptr runtimeConfig() const;

    /**
     * Handles everything needed when the hash of a file has changed between the
     * last write and the first read. This stores the new hash in a config file
     * and notifies implementing resources to handle a hash change if the
     * previous known hash was not empty. Finally this method clears the cache
     * and calls synchronize.
     * Returns true on success, false otherwise.
     */
    bool readLocalFile(const QString &fileName);

    /**
     * Reimplement to read your data from the given file.
     * The file is always local, loading from the network is done
     * automatically if needed.
     */
    virtual bool readFromFile(const QString &fileName) = 0;

    /**
     * Reimplement to write your data to the given file.
     * The file is always local, storing back to the network url is done
     * automatically when needed.
     */
    virtual bool writeToFile(const QString &fileName) = 0;

    /**
     * It is not always needed to parse the file when a resources is started.
     * (e.g. When the hash of the file is the same as the last time the resource
     * has written changes to the file). In this case setActualFileName is
     * called so that the implementing resource does know which file to read
     * when it actually needs to read the file.
     *
     * The default implementation will just call readFromFile( fileName ), so
     * implementing resources will have to explicitly reimplement this method to
     * actually get any profit of this.
     *
     * @p fileName This will always be a path to a local file.
     */
    virtual void setLocalFileName(const QString &fileName);

    /**
     * Generates the full path for the cache file in the case that a remote file
     * is used.
     */
    QString cacheFile() const;

    /**
     * Calculates an MD5 hash for given file. If the file does not exists
     * or the path is empty, this will return an empty QByteArray.
     */
    QByteArray calculateHash(const QString &fileName) const;

    /**
     * This method is called when the hash of the file has changed between the
     * last writeFile() and a readFile() call. This means that the file was
     * changed by another program.
     *
     * Note: This method is <em>not</em> called when the last known hash is
     *       empty. In that case it is assumed that the file is loaded for the
     *       first time.
     */
    virtual void handleHashChange();

    /**
     * Returns the hash that was stored to a cache file.
     */
    QByteArray loadHash() const;

    /**
     * Stores the given hash into a cache file.
     */
    void saveHash(const QByteArray &hash) const;

    /**
     * Returns whether the resource can be written to.
     */
    virtual bool readOnly() const = 0;

    /**
     * Returns the collection of this resource.
     */
    virtual Collection rootCollection() const = 0;

protected:
    QUrl mCurrentUrl;
    QStringList mSupportedMimetypes;
    QString mCollectionIcon;
    KIO::FileCopyJob *mDownloadJob = nullptr;
    KIO::FileCopyJob *mUploadJob = nullptr;
    QByteArray mCurrentHash;

protected Q_SLOTS:
    void scheduleWrite(); /// Called when changes are added to the ChangeRecorder.
    void handleProgress(KJob *, unsigned long);
    void fileChanged(const QString &fileName);
    void slotDownloadJobResult(KJob *);
    void slotUploadJobResult(KJob *);
};
}
