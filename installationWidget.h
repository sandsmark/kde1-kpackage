/////////////////////////////////////////////////////////////////////////////////
//      $Id: installationWidget.h 16726 1999-02-11 04:18:43Z toivo $ 
//
// Author: Toivo Pedaste
//
// This widget contains two sub-widgets, installOptions and packageDisplay.
// This widget is used to install packages.
/////////////////////////////////////////////////////////////////////////////////

#ifndef INSTALLATION_WIDGET
#define INSTALLATION_WIDGET

#include "config.h"
// Qt Headers
#include <qframe.h>
#include <qlayout.h>
#include <qdialog.h>

// ksetup headers
#include "packageInfo.h"
#include "packageDisplay.h"
#include "pkginstallOptions.h"

class pkgInterface;
class pkginstallOptionsWidgetMult;

class installationWidget : public QFrame
{
  Q_OBJECT;

  ///////////// METHODS ------------------------------------------------------
public:
  installationWidget(QWidget *parent=0, pkgInterface *type=0, const char *name=0);
  ~installationWidget();

  void installPackage(const char *location);

private:
  void setupWidgets(pkgInterface *type);
  ///////////// SLOTS --------------------------------------------------------
private slots:
  void finished(int refresh);
  ///////////// SIGNALS ------------------------------------------------------
signals:
  void finished(int mode, pkgInterface *interface, int refresh);
  ///////////// DATA ---------------------------------------------------------
private:
  QBoxLayout *layout;
  packageDisplayWidget *packageDisplay;
  pkgInterface *interface;
  pkginstallOptionsWidget *installOptions;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class pkginstallDialogMult : public QDialog
{
  Q_OBJECT;

public:
  pkginstallDialogMult(param *pars, QWidget *parent=0, const char *name=0);
  ~pkginstallDialogMult();

  void setup(QList<packageInfo> *pl, QString type);

  void uninstallClicked();

  pkginstallOptionsWidgetMult *installOptions;
  QBoxLayout *layout;

private slots:
  void finished(int refresh);
};
#endif
