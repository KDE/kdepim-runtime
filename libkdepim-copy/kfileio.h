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
		      bool withDialogs=true);

// unused
//QByteArray kFileToBytes(const QString &fileName, bool withDialogs=true);


/** Save a file. If withDialogs is FALSE no warning dialogs are opened if
 * there are problems. Returns TRUE on success and FALSE on failure.
 * Replaces existing files without warning if askIfExists==FALSE.
 * Makes a copy if the file exists to filename~ if createBackup==TRUE.
 */
bool kBytesToFile(const char* aBuffer, int len,
                  const QString &aFileName,
                  bool aAskIfExists, bool aBackup, bool aVerbose);

bool kCStringToFile(const QCString& buffer, const QString &fileName,
		   bool askIfExists=false, bool createBackup=true,
		   bool withDialogs=true);
/** Does not stop at NUL */
bool kByteArrayToFile(const QByteArray& buffer, const QString &fileName,
		   bool askIfExists=false, bool createBackup=true,
		   bool withDialogs=true);

}

#endif /*kpim_kfileio_h*/
