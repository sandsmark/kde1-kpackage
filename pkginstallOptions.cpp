/////////////////////////////////////////////////////////////////////////////////
//      $Id: pkginstallOptions.cpp 21211 1999-05-11 09:16:28Z bieker $ 
//
// File  : pkginstallOptions.cpp
// Author: Damyan Pepper
// Author: Toivo Pedaste
//

#include "../config.h"
// qt headers
#include <qlabel.h>

// ksetup headers
#include "pkginstallOptions.h"
#include "managementWidget.h"
#include "debInterface.h"
#include "kpackage.h"
#include <klocale.h>


// Constructor -- set the frame style, and setup the sub widgets
pkginstallOptionsWidget::pkginstallOptionsWidget(param *pars, QWidget *parent,
				    const char *name)
  : QFrame(parent,name)
{
  setFrameStyle(QFrame::Raised | QFrame::Panel);

  setupWidgets(pars);
  package = 0;
}

// Destructor
pkginstallOptionsWidget::~pkginstallOptionsWidget()
{
  int i;
  for (i = 0; i < bnumber; i++) {
    delete(Boxs[i]);
  }
}

// Set up the sub-widgets
void pkginstallOptionsWidget::setupWidgets(param *pars)
{
  int i;

  // Create widgets
  title = new QLabel(i18n("Install Package"), this);
  title->setFont(QFont("Helvetica",18, QFont::Bold));
  title->setAutoResize(TRUE);
  title->update();

  installButton = new QPushButton(i18n("Install"),this);
  cancelButton = new QPushButton(i18n("Cancel"),this);
  // count number of buttons
  for (bnumber = 0; pars[bnumber].name != 0; bnumber++);

  Boxs = new QCheckBox *[bnumber];
  for (i = 0; i < bnumber ; i++) {
    Boxs[i] = new QCheckBox(pars[i].name, this);
    Boxs[i]->setChecked(pars[i].init);
  }
  
  // Connections
  connect(installButton,SIGNAL(clicked()),SLOT(pkginstallButtonClicked()));
  connect(cancelButton,SIGNAL(clicked()),SLOT(cancelButtonClicked()));

  // Do the layout
  layout = new QBoxLayout(this,QBoxLayout::TopToBottom,bnumber);
  QBoxLayout *buttons = new QBoxLayout(QBoxLayout::LeftToRight);

  layout->addWidget(title,1);
  layout->addStretch(1);
  for (i = 0; i < bnumber; i++) {
    layout->addWidget(Boxs[i],1);
  }
  layout->addStretch(2);
  layout->addLayout(buttons);

#if QT_VERSION < 200
  buttons->addStrut(20);
#endif
  buttons->addWidget(installButton,2);
  buttons->addStretch(1);
  buttons->addWidget(cancelButton,2);

  layout->activate();
}

// Set the current package that is being worked with
void pkginstallOptionsWidget::setPackage(packageInfo *p)
{
  package = p;
}

// install button has been clicked....so install the package
void pkginstallOptionsWidget::pkginstallButtonClicked()
{
  // Disable the buttons
  int i, r;

  installButton->setEnabled(FALSE);
  cancelButton->setEnabled(FALSE);
  for (i = 0; i < bnumber; i++) {
    Boxs[i]->setEnabled(FALSE);
  }

  // Collect data from check boxes
  int installFlags = 0;
  
  for (i = 0; i < bnumber; i++) {
    installFlags |= (Boxs[i]->isChecked()) << i;
  }	


  //  interfaceFlags|=INSTALL_PERCENT;

  r = package->interface->install(installFlags, package);

  // enable the buttons
  installButton->setEnabled(TRUE);
  cancelButton->setEnabled(TRUE);
  for (i = 0; i < bnumber; i++) {
    Boxs[i]->setEnabled(TRUE);
  }

  // Emit the finished signal to indicate that installation is complete
  if (!r) {
    kpkg->kp->management->updatePackage(package,TRUE);
    emit finished(0);
  }
}

// Cancel button has been clicked -- finish without doing anything
void pkginstallOptionsWidget::cancelButtonClicked()
{
  kpkg->kp->management->updatePackage(package,TRUE);
  emit finished(0);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
pkginstallOptionsWidgetMult::pkginstallOptionsWidgetMult
                  (param *pars, QWidget *parent, const char *name)
  : pkginstallOptionsWidget(pars, parent, name)
{
  notModified = TRUE;
}

pkginstallOptionsWidgetMult::~pkginstallOptionsWidgetMult()
{
}

void pkginstallOptionsWidgetMult::setup(QList<packageInfo> *pl, QString type)
{
 QString s;

  packList = pl;
  s = i18n("Install: %1 %2 Packages").arg(packList->count()).arg(type);
  title->setText(s);
}

void pkginstallOptionsWidgetMult::pkginstallButtonClicked()
{
  int i, r;

  notModified = FALSE;

  // Collect data from check boxes
  int installFlags = 0;
  
  for (i = 0; i < bnumber; i++) {
    installFlags |= (Boxs[i]->isChecked()) << i;
  }	


  //  interfaceFlags|=INSTALL_PERCENT;

  r = packList->first()->interface->install(installFlags, packList);


  // Emit the finished signal to indicate that installation is complete
  if (!r) {
    emit finished(1);
  }
}

void pkginstallOptionsWidgetMult::cancelButtonClicked()
{
  if (notModified)
    emit finished(0);
  else
    emit finished(1);
}
