#ifndef KINPUTDIALOG_H
#define KINPUTDIALOG_H

#include <qstring.h>
#include <qstringlist.h>
#include <qinputdialog.h>

class KInputDialog : public QInputDialog
{
  public:
    static QString getItem( const QString &caption, const QString &label,
                            const QStringList &items, int current,
                            bool editable );
};

#endif
