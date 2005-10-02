/* Load / save entire (local) files with nice diagnostics dialog messages.
 * These functions load/save the whole buffer in one i/o call, so they
 * should be pretty efficient.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kpim_kfileio_h
#define kpim_kfileio_h

#include <kdepimmacros.h>

class QByteArray;
class QString;
class QWidget;

namespace KPIM {

/**
 * Loads the file with the given filename. Optionally, you can force the data
 * to end with a newline character. Moreover, you can suppress warnings.
 *
 * @param fileName      Name of the file that should be loaded.
 * @param ensureNewline If true, then the data will always have a trailing
 *                      newline. Defaults to true.
 * @param withDialogs   If false, then no warning dialogs are shown in case of
 *                      problems. Defaults to true.
 *
 * @return The contents of the file or an empty QByteArray if loading failed.
 */
QByteArray kFileToByteArray( const QString & fileName,
                             bool ensureNewline = true, 
                             bool withDialogs = true ) KDE_EXPORT;

/**
 * Writes the contents of @p buffer to the file with the given filename.
 *
 * @param buffer       The data you want to write to the file.
 * @param askIfExists  If true, then you will be asked before an existing file
 *                     is overwritten. If false, then an existing file is
 *                     overwritten without warning.
 * @param createBackup If true, then a backup of existing files will be
 *                     created. Otherwise, no backup will be made.
 * @param verbose      If true, then you will be warned in case of problems.
 *                     Otherwise, no warnings will be issued.
 *
 * @return True if writing the data to the file succeeded.
 * @return False if writing the data to the file failed.
 */
bool kByteArrayToFile( const QByteArray & buffer, const QString & fileName,
                       bool askIfExists = false, bool createBackup = true,
                       bool withDialogs = true ) KDE_EXPORT;

  /**
   * Checks and corrects the permissions of a file or folder, and if requested 
   * all files and folders below. It gives back a list of files which do not
   * have the right permissions. This list can be used to show to the user.
   *
   * @param toCheck         The file or folder of which the permissions should 
   *                        be checked.
   * @param recursive       Set to true, it will check the contents of a folder
   *                        for the permissions recursively. If false only 
   *                        toCheck will be checked.
   * @param wantItReadable  Set to true, it will check for read permissions. 
   *                        If the read permissions are not available, there will
   *                        be a attempt to correct this.
   * @param wantItWritable  Set to true, it will check for write permissions. 
   *                        If the write permissions are not available, there 
   *                        will be a attempt to correct this.
   * @return It will return a string with all files and folders which do not
   *         have the right permissions. If empty, then all permissions are ok.
   */
QString checkAndCorrectPermissionsIfPossible( const QString &toCheck,
   const bool recursive, const bool wantItReadable,
   const bool wantItWritable );

  /**
   * Checks and corrects the permissions of a file or folder, and if requested
   * all files and folders below. If the permissions are not ok, it tries to correct 
   * them. If that fails then a warning with detailled information is given.
   *
   * @param  parent         If parent is 0, then the message box becomes an
   *                        application-global modal dialog box. If parent 
   *                        is a widget, the message box becomes modal 
   *                        relative to parent.
   * @param toCheck         The file or folder of which the permissions should 
   *                        be checked.
   * @param recursive       Set to true, it will check the contents of a folder
   *                        for the permissions recursively. If false only 
   *                        toCheck will be checked.
   * @param wantItReadable  Set to true, it will check for read permissions. 
   *                        If the read permissions are not available, there will
   *                        be a attempt to correct this.
   * @param wantItWritable  Set to true, it will check for write permissions. 
   *                        If the write permissions are not available, there 
   *                        will be a attempt to correct this.
   * @return It will return true if all permissions in the end are ok. If false
   *         then the permissions are not ok and it was not possible to correct
   *         all errors.
   */
bool checkAndCorrectPermissionsIfPossibleWithErrorHandling( QWidget *parent, 
   const QString &toCheck, const bool recursive, const bool wantItReadable,
   const bool wantItWritable );
}

#endif /*kpim_kfileio_h*/
