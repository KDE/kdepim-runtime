#include "kinputdialog.h"

#include <qinputdialog.h>

QString KInputDialog::getItem( const QString &caption, const QString &label,
                               const QStringList &items, int current,
                               bool editable )
{
#ifndef QT_NO_INPUTDIALOG
  return QInputDialog::getItem( caption, label, items, current, editable );
#else
  return QString::null;
#endif
}
