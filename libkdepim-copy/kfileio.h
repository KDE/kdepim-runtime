/* Load / save entire (local) files with nice diagnostics dialog messages.
 * These functions load/save the whole buffer in one i/o call, so they
 * should be pretty efficient.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kpim_kfileio_h
#define kpim_kfileio_h

#include <qcstring.h>
#include <qwidget.h>

#include <kdepimmacros.h>

class QString;

namespace KPIM {

/** Load a file. Returns a pointer to the memory-block that contains
 * the loaded file. Returns a null string if the file could not be loaded.
 * If withDialogs is FALSE no warning dialogs are opened if there are
 * problems.
 * The string returned is always zero-terminated and therefore one
 * byte longer than the file itself.
 * If ensureNewline is TRUE the string will always have a trailing newline.
 */
QCString kFileToString(const QString &fileName, bool ensureNewline=true,
		      bool withDialogs=true) KDE_EXPORT;

// unused
//QByteArray kFileToBytes(const QString &fileName, bool withDialogs=true);


/** Save a file. If withDialogs is FALSE no warning dialogs are opened if
 * there are problems. Returns TRUE on success and FALSE on failure.
 * Replaces existing files without warning if askIfExists==FALSE.
 * Makes a copy if the file exists to filename~ if createBackup==TRUE.
 */
bool kBytesToFile(const char* aBuffer, int len,
                  const QString &aFileName,
                  bool aAskIfExists, bool aBackup, bool aVerbose) KDE_EXPORT;

bool kCStringToFile(const QCString& buffer, const QString &fileName,
		   bool askIfExists=false, bool createBackup=true,
		   bool withDialogs=true) KDE_EXPORT;
/** Does not stop at NUL */
bool kByteArrayToFile(const QByteArray& buffer, const QString &fileName,
		   bool askIfExists=false, bool createBackup=true,
		   bool withDialogs=true) KDE_EXPORT;


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
QString checkAndCorrectPermissionsIfPossible(const QString &toCheck,
   const bool &recursive, const bool &wantItReadable,
   const bool &wantItWritable);

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
   const QString &toCheck, const bool &recursive, const bool &wantItReadable,
   const bool &wantItWritable);
}

#endif /*kpim_kfileio_h*/
