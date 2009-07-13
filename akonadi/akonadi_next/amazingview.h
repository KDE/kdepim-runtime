
#ifndef AMAZINGVIEW_H
#define AMAZINGVIEW_H

#include <QListView>

#include "akonadi_next_export.h"

class AKONADI_NEXT_EXPORT AmazingView : public QListView
{
  Q_OBJECT
public:
  AmazingView(QWidget* parent = 0);

};

#endif