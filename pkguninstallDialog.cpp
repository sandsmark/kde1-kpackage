//////////////////////////////////////////////////////////////         
//      $Id: pkguninstallDialog.cpp 21211 1999-05-11 09:16:28Z bieker $ 
//
// Author: Toivo Pedaste
//
#include "config.h"
#include "pkguninstallDialog.h"
#include "pkgInterface.h"
#include "kpackage.h"
#include <klocale.h>

pkguninstallDialog::pkguninstallDialog(param *pars, QWidget *parent,
				       const char *name)
  : QDialog(parent,name,TRUE)
{
  int i;
  resize(300,200);

  // Create the widgets
  uninstallButton = new QPushButton(i18n("Uninstall"), this);
  cancelButton = new QPushButton(i18n("Cancel"), this);

  // count number of buttons
  bnumber = 0;
  for (i = 0; pars[i].name != 0; i++) {
    bnumber = i;
  }
  bnumber = i;

  Boxs = new QCheckBox *[bnumber];
  for (i = 0; i < bnumber; i++) {
    Boxs[i] = new QCheckBox(pars[i].name, this);
    Boxs[i]->setChecked(pars[i].init);
  }

  label = new QLabel("",this);
  label->setMargin(1);
  label->setFont(QFont("Helvetica",18, QFont::Bold));
  label->setAutoResize(TRUE);
  label->update();

  // Connections
  connect(uninstallButton,SIGNAL(clicked()), SLOT(uninstallClicked()));
  connect(cancelButton, SIGNAL(clicked()), SLOT(cancelClicked()));

  // Create the layout managers
  QBoxLayout *buttons;
  layout = new QBoxLayout(this, QBoxLayout::TopToBottom,5,0);
  buttons = new QBoxLayout(QBoxLayout::LeftToRight);


  // Setup layout
  layout->addWidget(label,2);
  layout->addStretch(1);
  for (i = 0; i < bnumber; i++) {
    layout->addWidget(Boxs[i],1);
  }
  layout->addStretch(1);
  layout->addLayout(buttons);

#if QT_VERSION < 200
  buttons->addStrut(20);
#endif
  buttons->addWidget(uninstallButton,2);
  buttons->addStretch(2);
  buttons->addWidget(cancelButton, 2);

  layout->activate();
}

pkguninstallDialog::~pkguninstallDialog()
{
  int i;
  for (i = 0; i < bnumber; i++) {
    delete(Boxs[i]);
  }
  delete []Boxs;
  delete uninstallButton;
  delete cancelButton;
  delete label;
  delete layout;
}

void pkguninstallDialog::setup(packageInfo *p)
{
  QString s;

  package = p;
  s.sprintf(i18n("Uninstall: %s\n"),
          package->getProperty("name")->data());
  label->setText(s.data());
}

void pkguninstallDialog::uninstallClicked()
{
  int i, r, uninstallFlags=0;

  for (i = 0; i < bnumber; i++) {
    uninstallFlags |= (Boxs[i]->isChecked()) << i;
  }	

  r = package->interface->uninstall(uninstallFlags,package);

  if (!r)
    accept();
}

void pkguninstallDialog::cancelClicked()
{
  reject();
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
pkguninstallDialogMult::pkguninstallDialogMult(param *pars, QWidget *parent,
					       const char *name)
  : pkguninstallDialog(pars, parent, name)
{
}

pkguninstallDialogMult::~pkguninstallDialogMult()
{
  notModified = TRUE;
}

void pkguninstallDialogMult::setup(QList<packageInfo> *pl, QString type)
{
  QString s;

  packList = pl;
  s.sprintf(i18n("Uninstall: %d %s Packages\n"),packList->count(),type.data());
  label->setText(s.data());
}

void pkguninstallDialogMult::uninstallClicked()
{
  int i, r, uninstallFlags=0;

  notModified = FALSE;

  for (i = 0; i < bnumber; i++) {
    uninstallFlags |= (Boxs[i]->isChecked()) << i;
  }	

  r = packList->first()->interface->uninstall(uninstallFlags,packList);

  if (!r)
    accept();
}

void pkguninstallDialogMult::cancelClicked()
{
  if (notModified)
    reject();
  else 
    accept();
}
