////////////////////////////////////////////////////////////////////////////////
// $Id: pkginstallOptions.h 16726 1999-02-11 04:18:43Z toivo $
// File  : pkginstallOptions.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This provides the installation options, plus the install and cancel
// buttons.  When the install button is clicked, the current package
// is installed.
/////////////////////////////

#ifndef PKGINSTALL_OPTIONS
#define PKGINSTALL_OPTIONS

#include "config.h"
// Qt headers
#include <qframe.h>
#include <qlayout.h>
#include <qpushbt.h>
#include <qlabel.h>
#include <qchkbox.h>

// kpackage headers
#include "packageInfo.h"
#include "pkgInterface.h"

struct param;

class pkginstallOptionsWidget : public QFrame
{
  Q_OBJECT;
public:
  pkginstallOptionsWidget( param *pars=0, QWidget *parent=0,
			  const char *name = 0);
  ~pkginstallOptionsWidget();

  // This sets the package that the widget is dealing with
  void setPackage(packageInfo *p);

  QCheckBox **Boxs;
  // options buttons

  int bnumber;
  // number of option buttons

  QLabel *title;
  // Widget title

private:
  // This sets up the sub-widgets
  void setupWidgets(param *pars);

private slots:  
  virtual void pkginstallButtonClicked();
  virtual void cancelButtonClicked();

signals:
  // This signal indicates that the widget has finished.
  void finished(int refresh);

private:
  // Pointer to the current package
  packageInfo *package;

  // The layout manager
  QBoxLayout *layout;

  // Sub widgets
  QPushButton *installButton, *cancelButton;

};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
class pkginstallOptionsWidgetMult : public pkginstallOptionsWidget
{
  Q_OBJECT;
public:
  pkginstallOptionsWidgetMult(param *pars, QWidget *parent = 0, const char *name = 0);
  ~pkginstallOptionsWidgetMult();

  void setup(QList<packageInfo> *pl, QString type);

  void pkginstallButtonClicked();

  void cancelButtonClicked();

  QList <packageInfo> *packList;
  // list of packages to install

  bool notModified;
  // haven't tried to do the install
};
#endif
