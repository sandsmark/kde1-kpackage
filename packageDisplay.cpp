////////////////////////////////////////////////////////////////////////////////
//      $Id: packageDisplay.cpp 21143 1999-05-09 22:54:36Z mueller $ 
// File  : packageDisplay.cpp
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
///////////////////////////////////////////////////////////////////////////////

#include "config.h"
// Standard headers
#include <stdio.h>

// Qt headers

#include <qapp.h>
#include <qfileinf.h> 

// kpackage.headers
#include <kiconloader.h> 

#include "kpackage.h"
#include "packageDisplay.h"
#include "packageProperties.h"
#include "pkgInterface.h"
#include "utils.h"
#include "options.h"
#include <klocale.h>

extern KIconLoader  *globalKIL; 
extern Params *params;

// constructor
packageDisplayWidget::packageDisplayWidget(QWidget *parent,
					   const char *name)
  : QFrame(parent,name)
{
  // Initially we're not dealing with any package
  package=NULL;

  // Set up the widgets
  setupWidgets();

  // Load the pixmaps
  tick = new QPixmap();
  *tick = globalKIL->loadIcon("ptick.xpm"); 

  cross = new QPixmap();
  *cross = globalKIL->loadIcon("cross.xpm"); 

  question = new QPixmap();
  *question = globalKIL->loadIcon("question.xpm");

  blank = new QPixmap();

}

packageDisplayWidget::~packageDisplayWidget()
{
  delete tick;
  delete cross;
  delete question;
}

void packageDisplayWidget::resizeEvent(QResizeEvent *re)
{
  arrangeWidgets();
  re = re;
}

void packageDisplayWidget::setupWidgets()
{
  tabbar = new QTabBar(this);
  QTab *proptab, *fltab;

  proptab = new QTab;
  fltab = new QTab;

  proptab->label = i18n("Properties");
  proptab->enabled = FALSE;
  fltab->label = i18n("File List");
  fltab->enabled = FALSE;

  packageProperties = new packagePropertiesWidget(this);
  fileList = new QListView(this);
  fileList->hide();
  fileList->addColumn("name");
  connect(fileList, SIGNAL(selectionChanged(QListViewItem *)),
            this, SLOT( openBinding(QListViewItem *)) );
  tabbar->addTab(proptab);
  tabbar->addTab(fltab);

  connect(tabbar,SIGNAL(selected(int)), SLOT(tabSelected(int)));
}

void packageDisplayWidget::arrangeWidgets()
{
  const int correction = 7;
  int th = tabbar->height();
  int h = height() - tabbar->height() + correction;

  tabbar->move(0,0);
  tabbar->resize(width(), th);
  packageProperties->move(0,th-correction);
  packageProperties->resize(width(),h);
  fileList->move(0,th-correction);
  fileList->resize(width(),h);
}

void packageDisplayWidget::tabSelected(int tab)
{
  if(tab==0)
    {
      packageProperties->show();
      fileList->hide();
    }
  else
    {
      packageProperties->hide();
      if (!initList) {
	updateFileList();
	initList = 1;
      }
      fileList->show();
    }
}

void packageDisplayWidget::noPackage()
{
  if (tabbar->isTabEnabled(1))
    tabbar->setTabEnabled(1,FALSE);
  if (tabbar->isTabEnabled(0))
    tabbar->setTabEnabled(0,FALSE);
  packageProperties->changePackage(NULL);
  //  fileList->collapseItem(0);
  fileList->clear();
}

// Change packages
void packageDisplayWidget::changePackage(packageInfo *p)
{
  int tfl = fileList->isVisible();

  if (package && !package->getItem()) {
    delete package;
    package = 0;
  }

  package = p;
  if (!p) {			// change to no package
    noPackage();
  } else {
    QString u = package->getFilename();
    if (!package->updated &&  !u.isEmpty()) {
      packageInfo *np = package->interface->getPackageInfo('u', u.data(), 0);

      if (np) {
	QDictIterator<QString> it(*(np->getDict())); // update info entries in p
	while (it.current()) {
	  package->info->replace(it.currentKey(),new QString(it.current()->data()));
	  ++it;
	}
	delete np;
	package->updated = TRUE;
      }
    }

    initList = 0;
    packageProperties->changePackage(package);
    tabbar->setTabEnabled(0,TRUE);
    tabbar->setTabEnabled(1,TRUE);
    tabSelected(tfl);          
  }
}

// update the file list
void packageDisplayWidget::updateFileList()
{
  // Get a list of files in the package
  QList<char> *files = 0;
  QList<char> *errorfiles = 0;

  // set the status
  kpkg->kp->setStatus(i18n("Updating File List"));

  // clear the file list
  fileList->clear();

  // Check if the package is installed
  int installed;
  if(package->getFilename().isEmpty()) {
    if(package->packageState == packageInfo::UPDATED) {
      fileList->setColumnText(0, "");
      return;
    } else
      installed=1;
  } else
    installed=0;

  files = package->interface->getFileList(package);

  if(!files)
    return;

  // Get a list of files that failed verification
  if(installed && params->VerifyFL) {
    errorfiles = package->interface->verify(package, files);
  } else
    errorfiles=NULL;

  kpkg->kp->setStatus(i18n("Updating File List"));
  
  // Need a list iterator
  QListIterator<char> it(*files);

  char *s;
  uint c=0, p=0;
  uint step = (it.count() / 100) + 1;

  QString ftmp;
  ftmp.setNum(it.count());
  ftmp += i18n(" files");

  fileList->setColumnText(0, ftmp.data());

  QListViewItem *q;

  // Go through all the files
  for (; (s = it.current()); ++it) {
    // Update the status progress
    c++;
    if(c > step) {
      c=0;
      p++;
      kpkg->kp->setPercent(p);
    }

    int error=0;
    char *t;
    if(installed && errorfiles) { // see if file failed verification,
      for(t=errorfiles->first(); t!=0; t=errorfiles->next()) {
	if(strcmp(s,t)==0) {
	  error = 1;
	}
      }

      // insert file in tree list with correct pixmap
      if(error) {
	q =  new QListViewItem(fileList,s);
	q->setPixmap(0,*cross);
      } else {
	q =  new QListViewItem(fileList,s);
	q->setPixmap(0,*tick);
      }
    } else {
      q =  new QListViewItem(fileList,s);
      q->setPixmap(0,*question);
    }
  }
  
  if(files) {
    delete files;
  }
  if(errorfiles) {
    delete errorfiles;
  }

  kpkg->kp->setPercent(100);
}

void packageDisplayWidget::openBinding(QListViewItem *index)
  {
    QString tmp= "file:";
    KFM *kfm= new KFM();

    QFileInfo *fileInfo =
      new QFileInfo(index->text(0));

    if (fileInfo->isDir())
      {
	tmp.append(fileInfo->filePath());
	kfm->openURL(tmp.data());
      }
    else
      {
	tmp.append(index->text(0));
	kfm->exec(tmp.data(),0L);
      };
    delete kfm;
  };
