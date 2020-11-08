///////////////////////////////////////////////////////////////////////////////
//      $Id: installationWidget.cpp 20996 1999-05-07 14:13:11Z toivo $ 
//
// Author: Toivo Pedaste
// 
// Two panel installation widget
//

#include "../config.h"
// KDE headers
#include <kurl.h>

// kpackage.headers
#include "kpackage.h"
#include "installationWidget.h"
#include "pkginstallOptions.h"
#include <klocale.h>

// Constructor -- setup the widgets
installationWidget::installationWidget(QWidget *parent,
			  pkgInterface *type, const char *name)
  : QFrame(parent, name)
{
  setupWidgets(type);
}

// Destructor
installationWidget::~installationWidget()
{
}

// Setup the widgets
void installationWidget::setupWidgets(pkgInterface *type)
{
  interface = type;

  // Create the widgets
  packageDisplay = new packageDisplayWidget(this);
  installOptions = new pkginstallOptionsWidget(type->initinstallOptions(), this);

  // Connections
  connect(installOptions,SIGNAL(finished(int)),SLOT(finished(int)));

  // Set up the layout manager
  layout = new QBoxLayout(this, QBoxLayout::LeftToRight);

  layout->addWidget(installOptions, 1);
  layout->addSpacing(2);
  layout->addWidget(packageDisplay, 3);

  layout->activate();
}

// This tell installationWidget to start the process of installing
// a package from location.  It sets up the sub widgets -- installOptions
// actually installs the package.

void installationWidget::installPackage(const char *location)
{
  packageInfo *l;
  
  const char *filename = location;
  
  l = interface->getPackageInfo('u',filename, 0);
  packageInfo *p = l;

  // If it was got ok...
  if(p)
    {
      // Set the filename in the package to filename
      p->setFilename(filename);

      // Tell the other widgets to worry about installing this package
      packageDisplay->changePackage(p);
      installOptions->setPackage(p);
    }
  else
    {
      // Cannot install this package, so just give up
      emit finished(KPACKAGE::Installation, interface,0);
    }

  // Set the status
  QString s;
  s.sprintf(i18n("Install: %s"),location);
  kpkg->kp->setStatus(s);
  kpkg->kp->setPercent(100);

}

// Finished installation mode -- so emit signal saying so.
void installationWidget::finished(int refresh)
{
 emit finished(KPACKAGE::Installation, interface, refresh);
 if (kpkg->urlList.count() > 0) {
   kpkg->kp->openNetFile(kpkg->urlList.first());
   kpkg->urlList.removeFirst();
 }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

pkginstallDialogMult::pkginstallDialogMult(param *pars, 
			     QWidget *parent, const char *name)
  : QDialog(parent,name,TRUE)
{
  resize(250,250);

  installOptions = new pkginstallOptionsWidgetMult
                     (pars, this);

  connect(installOptions,SIGNAL(finished(int)),SLOT(finished(int))); 

  layout = new QBoxLayout(this, QBoxLayout::LeftToRight);

  layout->addWidget(installOptions, 1);

  layout->activate();
}

pkginstallDialogMult::~pkginstallDialogMult()
{
}

void pkginstallDialogMult::setup(QList<packageInfo> *pl, QString type)
{
  installOptions->setup(pl,type);
}

void pkginstallDialogMult::finished(int refresh)
{
  if (refresh)
    accept();
  else
    reject();
}
