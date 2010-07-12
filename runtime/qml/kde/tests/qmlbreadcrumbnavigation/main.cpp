
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  MainWindow mw;
  mw.resize(640, 480);
  mw.show();

  return app.exec();

}


