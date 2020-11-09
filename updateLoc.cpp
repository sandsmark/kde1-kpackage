//////////////////////////////////////////////////////////////         
//      $Id: updateLoc.cpp 21211 1999-05-11 09:16:28Z bieker $ 
//
// Author: Toivo Pedaste
//
#include "config.h"

#include "kpackage.h"
#include "updateLoc.h"
#include "pkgInterface.h"
#include "options.h"
#include "cache.h"
#include <klocale.h>

extern Params *params;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
dpanel::dpanel(updateLoc *upd,  const char *Pfilter, QWidget *parent,
           bool buse, const char *name )
  : QWidget( parent, name )   
{
  filter = Pfilter;
  puse = 0;

  if (buse && upd->haveBase) {
    base = upd->base;
  } else {
    base = 0;
  }
    
  pack = new QHBoxLayout(this);
  {
    if (buse) {
      puse = new QCheckBox(i18n("Use"),this);
#if QT_VERSION < 200
      puse->setFixedSize(puse->sizeHint());
#endif
      pack->addWidget(puse,0);
    }

    pent = new QLineEdit(this);
#if QT_VERSION < 200
    pent->setFixedHeight(pent->sizeHint().height());
#endif
    pent->setMinimumWidth(280);
    pack->addWidget(pent,0);

    pbut = new QPushButton("...",this);
#if QT_VERSION < 200
    pbut->setFixedSize(pbut->sizeHint());
#endif
    pack->addWidget(pbut,0);

#if QT_VERSION < 200
    this->setFixedHeight(pent->sizeHint().height());
#endif
    if (upd->haveBase && buse)
      connect(pbut, SIGNAL(clicked()), this, SLOT(fileOpen()));
    else
      connect(pbut, SIGNAL(clicked()), this, SLOT(dirOpen()));

  }
}

dpanel::~dpanel()
{
  if (puse)
    delete puse;
  delete pent;
  delete pack;
  delete pbut;
}

QString dpanel::getText()
{
  QString s = pent->text();
  return s;
}

void dpanel::setText(const char *s)
{
 pent->setText(s);
}

bool dpanel::getUse()
{
  if (puse)
    return puse->isChecked();
  else
    return FALSE;
}

void dpanel::setUse(int n)
{
  if (n)
    puse->setChecked(TRUE);
  else
    puse->setChecked(FALSE);
}

KFileDialog* dpanel::getFileDialog(const char* captiontext, bool dir)
{
  QString st;

  if (base && getText().isEmpty()) {
    st = base->getText();
  } else {
    st = getText();
  }
  if (!dir) {
    if (st.right(8) == "Packages") {
      st.truncate(st.length() - 8);
    }
  }

  KFileDialog *indexDialog = new KFileDialog(st, filter,
				  this,"indexDialog",TRUE);

  indexDialog->setCaption(captiontext);
  indexDialog->rereadDir();

  return indexDialog;
}

void dpanel::fileOpen()
{
    QString fname;
    KFileDialog *box;

    box = getFileDialog(i18n("Package file"),FALSE);
    
    box->show();
    
    if (!box->result())   /* cancelled */
      return;
    if(box->selectedFile().isEmpty()) {  /* no selection */
      return;
    }
    
    fname =  box->selectedFileURL();
    if (fname.left(5) == "file:") {
      fname.remove(0,5);
    }

    delete box;

    pent->setText(fname);
}

void dpanel::dirOpen()
{
    QString fname;
    KFileDialog *box;

    box = getFileDialog(i18n("Package archive"),TRUE);
    
    box->show();
    
    if (!box->result())   /* cancelled */
      return;
    if(box->selectedFile().isEmpty()) {  /* no selection */
      return;
    }
    
    fname =  box->selectedFileURL();
    if (fname.left(5) == "file:") {
      fname.remove(0,5);
    }

    pent->setText(fname);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
updateLoc::updateLoc(QWidget *p, int panelNumber, int numberLines,
             pkgInterface *inter, const char *msg,
             const char *iname, const char *filter, const char *lmsg, const char *bmsg)
    : QDialog(p,"updateLoc",FALSE)
{
  interName = iname;
  interface = inter;
  panNumber = panelNumber;
  if (bmsg)
    haveBase = TRUE;
  else
    haveBase = FALSE;
  if (numberLines > PNUM)
    numLines = PNUM;
  else
    numLines = numberLines;

  QString pn;
  pn.setNum(panNumber);
  pn += "_";

  packL = "Package_L_";
  packL += pn;
  packU = "Package_U_";
  packU += pn;
  availB = "Available_Base";
  availB += pn;

  vtop = new QVBoxLayout( this, 10, 10, "vtop");
  {
    {
      frame1 = new QGroupBox(msg, this, "frame1");
      vtop->addWidget(frame1,1);
      vf = new QVBoxLayout( frame1, 15, 10, "vf");
      {
	//	hloc = new QHBoxLayout(frame1);

	if (haveBase) {
	  fbase = new QGroupBox(bmsg, frame1);
	  vbase = new QVBoxLayout(fbase, 15, 10, "vbase");
	  vf->addWidget(fbase,1);
	  base = new dpanel(this, filter,  fbase, FALSE);
	  vbase->addWidget(base,0);
	  vbase->activate();
	}
	floc = new QGroupBox(lmsg, frame1);
	vf->addWidget(floc,1);
	vloc = new QVBoxLayout(floc, 15, 10, "vloc");
	for (int i = 0; i < numLines; i++) {
	  dp[i] = new dpanel(this, filter,  floc);
	  vloc->addWidget(dp[i],0);
	}
      }
      vloc->activate();
    }
    vf->activate();
    vtop->activate();
  }
  readSettings();
#if QT_VERSION < 200
  resize(wdth,minimumSize().height());
#endif
}

updateLoc::~updateLoc()
{
  for (int i = 0; i < numLines; i++) {
    delete dp[i];
  }
}

void updateLoc::restore() 
{
  readSettings();
  show();
}

void updateLoc::applyS(LcacheObj *slist) 
{
  QString t,pn,cn,pv,prev;
  cacheObj *CObj;

  KConfig *config = app->getConfig();
  config->setGroup(interName);

  cn = interface->head.data();
  cn += "_";
  cn += pn.setNum(panNumber);
  cn += "_";

  for (int i = 0; i < numLines; i++) {
    // delete chached dir if text changed
    pv = packL + pn.setNum(i); 
    prev = config->readEntry(pv,"");
    if (prev != dp[i]->getText())
      cacheObj::rmDCache(QString(cn + pn.setNum(i)));

    // create cache object corresponding to this entry
    if (dp[i]->getUse()) {
      t = dp[i]->getText();
      if (!t.isEmpty()) {
	if (t.left(5) == "file:") {
	  t.remove(0,5);
	}
	CObj = new cacheObj(haveBase ? base->getText() : (QString)"-",
			    t, cn + pn.setNum(i));
	slist->append(CObj);
	//	printf("T=%s\n",t.data());
      }
    } 
  }
  writeSettings();
}

void updateLoc::readSettings()
{
  QString pv, pn;
  
  KConfig *config = app->getConfig();

  config->setGroup(interName);

  if (haveBase)
    base->setText(config->readEntry(availB));

  for (int i = 0; i < numLines; i++) {
    pv = packL + pn.setNum(i);
    dp[i]->setText(config->readEntry(pv,""));
    pv = packU + pn.setNum(i);
    dp[i]->setUse(config->readNumEntry(pv));

  }
  wdth = config->readNumEntry("Width",500);
}

void updateLoc::writeSettings()
{
  QString pv, pn;

  KConfig *config = app->getConfig();

  config->setGroup(interName);
  if (haveBase) {
    config->writeEntry(availB,base->getText());
  }

  for (int i = 0; i < numLines; i++) {
    pv = packL + pn.setNum(i);
    config->writeEntry(pv,dp[i]->getText());
    pv = packU + pn.setNum(i);
    config->writeEntry(pv,(int)dp[i]->getUse());
  }
  config->writeEntry("Width",width());
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Locations::Locations( const char *msg)
    : QDialog(0,"updateLoc",FALSE)
{
  QString nm, mp;

  numPanels = 0;

  tab = new QTabDialog(0,"Locations");

  tab->setCaption(msg);

  tab->setCancelButton( i18n("Cancel") );
  tab->setApplyButton( i18n("Apply") );
  tab->setOkButton( i18n("OK") );

  connect( tab, SIGNAL(applyButtonPressed()), SLOT(apply_slot()) );
}

void Locations::dLocations(int numberDirs,  int numberLines,
             pkgInterface *inter, const char *label,
             const char *iname, const char *filter, const char *dirMsg)
{
  QString nm;

  for (int i = numPanels ; i <  numPanels + numberDirs; i++) {
    pn[i] = new updateLoc(tab,i,numberLines,inter, "", iname,
		      filter, dirMsg);
    QString mp = label;
    nm.setNum(i+1);
    mp += nm;
    tab->addTab(pn[i],mp);
  }
  numPanels += numberDirs;
}

void Locations::pLocations(int numberDirs,  int numberLines,
             pkgInterface *inter, const char *label, const char *iname,
             const char *filter,
		     const char *packMsg, const char *baseMsg)
{
  QString nm;

  for (int i = numPanels; i < numPanels + numberDirs; i++) {
    pn[i] = new updateLoc(tab,i,numberLines,inter, "", iname,
		      filter, packMsg, baseMsg);
    QString mp = label;
    nm.setNum(i+1);
    mp += nm;
    tab->addTab(pn[i],mp.data());
  }
  numPanels += numberDirs;
}

Locations::~Locations() {
  for (int i = 0; i <  numPanels; i++) {
    delete pn[i];
  }
}

void Locations::restore() 
{
  tab->show();
}

void Locations::apply_slot() 
{
   LcacheObj *slist = new LcacheObj();

  for (int i = 0; i <  numPanels; i++) {
    pn[i]->applyS(slist);
  }
  emit returnVal(slist);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
