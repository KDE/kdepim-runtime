#include "kpixmapregionselectordialog.h"
#include <kdialogbase.h>
#include <qdialog.h>
#include <qdesktopwidget.h>
#include <klocale.h>

using namespace KPIM;

KPixmapRegionSelectorDialog::KPixmapRegionSelectorDialog(QWidget *parent,
     const char *name, bool modal ) : KDialogBase(parent, name, modal, i18n("Select a region of the Image"), Help|Ok|Cancel, Ok, true )
{
  m_pixmapSelectorWidget= new KPixmapRegionSelectorWidget(this);

  setMainWidget(m_pixmapSelectorWidget);
}

KPixmapRegionSelectorDialog::~KPixmapRegionSelectorDialog()
{
}

QRect KPixmapRegionSelectorDialog::getSelectedRegion(const QPixmap &pixmap, QWidget *parent )
{
  KPixmapRegionSelectorDialog dialog(parent);

  dialog.pixmapRegionSelectorWidget()->setPixmap(pixmap);

  QDesktopWidget desktopWidget;
  QRect screen=desktopWidget.availableGeometry();
  dialog.pixmapRegionSelectorWidget()->setMaximumWidgetSize(
        (int)(screen.width()*4.0/5), (int)(screen.height()*4.0/5));

  int result = dialog.exec();

  QRect rect;

  if ( result == QDialog::Accepted )
    rect = dialog.pixmapRegionSelectorWidget()->unzoomedSelectedRegion();

  return rect;
}

QRect KPixmapRegionSelectorDialog::getSelectedRegion(const QPixmap &pixmap, int aspectRatioWidth, int aspectRatioHeight, QWidget *parent )
{
  KPixmapRegionSelectorDialog dialog(parent);

  dialog.pixmapRegionSelectorWidget()->setPixmap(pixmap);
  dialog.pixmapRegionSelectorWidget()->setSelectionAspectRatio(aspectRatioWidth,aspectRatioHeight);

  QDesktopWidget desktopWidget;
  QRect screen=desktopWidget.availableGeometry();
  dialog.pixmapRegionSelectorWidget()->setMaximumWidgetSize(
        (int)(screen.width()*4.0/5), (int)(screen.height()*4.0/5));

  int result = dialog.exec();

  QRect rect;

  if ( result == QDialog::Accepted )
    rect = dialog.pixmapRegionSelectorWidget()->unzoomedSelectedRegion();

  return rect;
}

QImage KPixmapRegionSelectorDialog::getSelectedImage(const QPixmap &pixmap, QWidget *parent )
{
  KPixmapRegionSelectorDialog dialog(parent);

  dialog.pixmapRegionSelectorWidget()->setPixmap(pixmap);

  QDesktopWidget desktopWidget;
  QRect screen=desktopWidget.availableGeometry();
  dialog.pixmapRegionSelectorWidget()->setMaximumWidgetSize(
        (int)(screen.width()*4.0/5), (int)(screen.height()*4.0/5));
  int result = dialog.exec();

  QImage image;

  if ( result == QDialog::Accepted )
    image = dialog.pixmapRegionSelectorWidget()->selectedImage();

  return image;
}

QImage KPixmapRegionSelectorDialog::getSelectedImage(const QPixmap &pixmap, int aspectRatioWidth, int aspectRatioHeight, QWidget *parent )
{
  KPixmapRegionSelectorDialog dialog(parent);

  dialog.pixmapRegionSelectorWidget()->setPixmap(pixmap);
  dialog.pixmapRegionSelectorWidget()->setSelectionAspectRatio(aspectRatioWidth,aspectRatioHeight);

  QDesktopWidget desktopWidget;
  QRect screen=desktopWidget.availableGeometry();
  dialog.pixmapRegionSelectorWidget()->setMaximumWidgetSize(
        (int)(screen.width()*4.0/5), (int)(screen.height()*4.0/5));

  int result = dialog.exec();

  QImage image;

  if ( result == QDialog::Accepted )
    image = dialog.pixmapRegionSelectorWidget()->selectedImage();

  return image;
}

