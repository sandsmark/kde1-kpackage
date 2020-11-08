//////////////////////////////////////////////////////////////         
//      $Id: options.cpp 19960 1999-04-17 22:36:57Z coolo $ 
//
// Author: Toivo Pedaste
//
#include "config.h"

#include "kpackage.h"
#include "managementWidget.h"
#include "pkgInterface.h"
#include "options.h"
#include "cache.h"
#include <klocale.h>

extern Params *params;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Options::Options(QWidget *parent, const char *name)
    : QDialog(parent, name,FALSE){

    readSettings();
    if (DCache >= Params::SESSION) {
      cacheObj::clearDCache(); // clear dir caches if needed
    }
    if (PCache >= Params::SESSION) {
      cacheObj::clearPCache(); // clear package caches if needed
    }

    tab = new QTabDialog(0,"Options");
    tab->setCaption("Options");

    {
      bg = new QButtonGroup(tab);
      bg->setTitle(i18n("Packages to display"));
      connect( bg, SIGNAL(clicked(int)), SLOT(PDisplay(int)) );
      QVBoxLayout* vb = new QVBoxLayout( bg, 15, 10, "vb");
      vb->addSpacing( bg->fontMetrics().height() );

      disp[0] = new QRadioButton(i18n("Installed Packages"),bg);
      vb->addWidget(disp[0]);
#if QT_VERSION < 200
      disp[0]->setMinimumSize( disp[0]->sizeHint() );
#endif

      disp[1] = new QRadioButton(i18n("Updated Packages"),bg);
      vb->addWidget(disp[1]);
#if QT_VERSION < 200
      disp[1]->setMinimumSize( disp[1]->sizeHint() );
#endif

      disp[2] = new QRadioButton(i18n("New and Updated  Packages"),bg);
      vb->addWidget(disp[2]);
#if QT_VERSION < 200
      disp[2]->setMinimumSize( disp[2]->sizeHint() );
#endif

      disp[3] = new QRadioButton(i18n("All Packages"),bg);
      vb->addWidget(disp[3]);
#if QT_VERSION < 200
      disp[3]->setMinimumSize( disp[3]->sizeHint() );
#endif

      disp[displayP]->setChecked(TRUE);
#if QT_VERSION < 200
      bg->setMinimumSize( bg->childrenRect().size() );
      vb->activate();
#endif
    }
    tab->addTab(bg,i18n("Package Display"));

    {
      bc = new QButtonGroup(tab);
      bc->setTitle(i18n("Cache remote package directories"));
      connect( bc, SIGNAL(clicked(int)), SLOT(PDCache(int)) );

      QVBoxLayout* vc = new QVBoxLayout( bc, 15, 10, "vc");
      vc->addSpacing( bc->fontMetrics().height() );

      dcache[0] = new QRadioButton(i18n("Always"),bc);
      vc->addWidget(dcache[0]);
#if QT_VERSION < 200
      dcache[0]->setMinimumSize( dcache[0]->sizeHint() );
#endif

      dcache[1] = new QRadioButton(i18n("During as session"),bc);
      vc->addWidget(dcache[1]);
#if QT_VERSION < 200
      dcache[1]->setMinimumSize( dcache[1]->sizeHint() );
#endif

      dcache[2] = new QRadioButton(i18n("Never"),bc);
      vc->addWidget(dcache[2]);
#if QT_VERSION < 200
      dcache[2]->setMinimumSize( dcache[2]->sizeHint() );
#endif

      if (DCache > 2)
	DCache = 2;
      dcache[DCache]->setChecked(TRUE);
#if QT_VERSION < 200
      bc->setMinimumSize( bc->childrenRect().size() );
      vc->activate();
#endif
    }
    tab->addTab(bc,i18n("Directory Cache"));

    {
      bp = new QButtonGroup(tab);
      bp->setTitle(i18n("Cache remote package files"));
      connect( bp, SIGNAL(clicked(int)), SLOT(PPCache(int)) );

      QVBoxLayout* vp = new QVBoxLayout( bp, 15, 10, "vp");
      vp->addSpacing( bp->fontMetrics().height() );

      pcache[0] = new QRadioButton(i18n("Always"),bp);
      vp->addWidget(pcache[0]);
#if QT_VERSION < 200
      pcache[0]->setMinimumSize( pcache[0]->sizeHint() );
#endif

      pcache[1] = new QRadioButton(i18n("During as session"),bp);
      vp->addWidget(pcache[1]);
#if QT_VERSION < 200
      pcache[1]->setMinimumSize( pcache[1]->sizeHint() );
#endif

      pcache[2] = new QRadioButton(i18n("Never"),bp);
      vp->addWidget(pcache[2]);
#if QT_VERSION < 200
      pcache[2]->setMinimumSize( pcache[2]->sizeHint() );
#endif

      if (PCache > 2)
	PCache = 2;
      pcache[PCache]->setChecked(TRUE);
#if QT_VERSION < 200
      bp->setMinimumSize( bp->childrenRect().size() );
      vp->activate();
#endif
    }
    tab->addTab(bp,i18n("Package Cache"));

    {
      framem = new QGroupBox("", tab);
      QVBoxLayout* vf = new QVBoxLayout(framem,20,20);
      valid = new QCheckBox(i18n("Verify file list"), framem, "valid");
      vf->addWidget(valid,0,AlignLeft);
#if QT_VERSION < 200
      valid->setFixedSize(valid->sizeHint());
#endif
      valid->setChecked(verifyFL);
      pkgRead = new QCheckBox(i18n("Read information from all local package files"), framem, "pkgr");
      vf->addWidget(pkgRead,0,AlignLeft);
#if QT_VERSION < 200
      pkgRead->setFixedSize(pkgRead->sizeHint());
#endif
      pkgRead->setChecked(PkgRead);
    }
    tab->addTab(framem,i18n("misc"));

    tab->setCancelButton( i18n("Cancel") );
    tab->setApplyButton( i18n("Apply") );
    tab->setOkButton( i18n("OK") );
#if QT_VERSION < 200
    tab->resize(minimumSize());
#endif

    connect( tab, SIGNAL(applyButtonPressed()), SLOT(apply_slot()) );
}

Options::~Options()
{
}

void Options::restore() 
{
  readSettings();
  tab->show();
}

void Options::apply_slot() 
{
  params->VerifyFL = valid->isChecked();
  params->PkgRead = pkgRead->isChecked();

  params->DisplayP = dp;
  if (dp != displayP)
    kpkg->kp->reload();
  displayP =dp;

  params->DCache = dc;
  params->PCache = pc;

  writeSettings();
}

void Options::PDisplay(int r)
{
  dp = r;
}

void Options::PDCache(int r)
{
  dc = r;
}

void Options::PPCache(int r)
{
  pc = r;
}

void Options::readSettings()
{
  
  KConfig *config = app->getConfig();

  config->setGroup("Kpackage");

  displayP = config->readNumEntry("Package_Display",3);
  params->DisplayP = displayP;
  dp = displayP;

  DCache = config->readNumEntry("Dir_Cache",1);
  params->DCache = DCache;
  dc = DCache;

  PCache = config->readNumEntry("Package_Cache",0);
  params->PCache = PCache;
  pc = PCache;

  verifyFL = config->readNumEntry("Verify_File_List",1);
  params->VerifyFL = verifyFL;

  PkgRead = config->readNumEntry("Read_Package_files",0);
  params->PkgRead = PkgRead;

}

void Options::writeSettings()
{
  
  KConfig *config = app->getConfig();

  config->setGroup("Kpackage");

  config->writeEntry("Package_Display", dp);
  config->writeEntry("Dir_Cache", dc);
  config->writeEntry("Package_Cache", pc);
  config->writeEntry("Verify_File_List", valid->isChecked());

  config->writeEntry("Read_Package_files", pkgRead->isChecked());
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Params::Params()
{
}

Params::~Params()
{
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
