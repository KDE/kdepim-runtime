#ifndef KINPUTDIALOG_H
#define KINPUTDIALOG_H

#include <qstring.h>
#include <qstringlist.h>

class KInputDialog
{
  public:
    static QString getItem( const QString &caption, const QString &label,
                            const QStringList &items, int current,
                            bool editable );
};

#endif
