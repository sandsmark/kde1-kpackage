///////////////////////////////////////////////////////////////////////////
// $Id: managementWidget.cpp 21066 1999-05-08 19:49:53Z bieker $	
// File  : managementWidget.cpp
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// See managementWidget.h for more information

#include "config.h"

#include "knewpanner.h"
// kpackage.headers
#include "kpackage.h"
#include "ktvitem.h"
#include "managementWidget.h"
#include "pkgInterface.h"
#include "pkguninstallDialog.h"
#include "packageDisplay.h"
#include "packageProperties.h"
#include "installationWidget.h"
#include <klocale.h>

// constructor -- initialise variables
managementWidget::managementWidget(QWidget *parent, const char *name)
  : QFrame(parent,name)
{
  package=NULL;
  installedPackages=NULL;
  dirInstPackages = new QDict<packageInfo>(7717);
  dirUninstPackages = new QDict<packageInfo>(7717);
  dirInfoPackages = new QDict<packageInfo>(7717);
  setupWidgets();
}

managementWidget::~managementWidget()
{
  //  if(installedPackages)
  delete installedPackages;
  delete dirInfoPackages;
  delete dirInstPackages;
  delete dirUninstPackages;
}

void managementWidget::resizeEvent(QResizeEvent *re)
{
    arrangeWidgets();
}

void managementWidget::closeEvent(QCloseEvent *)
{
    installedPackages->clear();

}


void managementWidget::setupWidgets()
{
  // Create the widgets

  top = new QBoxLayout(this,QBoxLayout::TopToBottom);
  vPan  = new KNewPanner(this, "vertPanner", KNewPanner::Vertical,
			 KNewPanner::Absolute);
  // the left panel
  top->addWidget(vPan);
  treeList = new QListView(vPan);
  treeList->setFrameStyle(QFrame::Panel|QFrame::Sunken);
  treeList->setLineWidth(2);
  treeList->setItemMargin(2);
  treeList->addColumn("Package");
  treeList->setColumnWidthMode(0,QListView::Manual);
  treeList->addColumn("Size");
  treeList->setColumnWidthMode(1,QListView::Manual);
  treeList->addColumn("Version");
  treeList->setColumnWidthMode(2,QListView::Manual);
  treeList->addColumn("Old Version");
  treeList->setColumnWidthMode(3,QListView::Manual);
  treeList->setAllColumnsShowFocus(TRUE);
  treeList->setRootIsDecorated(TRUE);
  readTreePos();

  connect(treeList, SIGNAL(selectionChanged(QListViewItem *)),
	  SLOT(packageHighlighted(QListViewItem *)));
  connect(treeList, SIGNAL(currentChanged(QListViewItem *)),
	  this, SLOT(currentChanged(QListViewItem *)));

  // the right panel
  rightpanel = new QFrame(vPan);
  rightbox = new QBoxLayout(rightpanel,QBoxLayout::TopToBottom);

  packageDisplay = new packageDisplayWidget(rightpanel);
  packageDisplay->setFrameStyle(QFrame::Panel|QFrame::Sunken);
  packageDisplay->setLineWidth(2);
  connect(this, SIGNAL(changePackage(packageInfo *)),
	  packageDisplay, SLOT(changePackage(packageInfo *)));

  buttons = new QBoxLayout(QBoxLayout::LeftToRight);

  uinstButton = new QPushButton(i18n("Uninstall"),rightpanel);
  uinstButton->setEnabled(FALSE);
  instButton = new QPushButton(i18n("Install"),rightpanel);
  instButton->setEnabled(FALSE);


  // Setup the `right panel' layout
  rightbox->addWidget(packageDisplay,10);
  rightbox->addSpacing(5);
  rightbox->addLayout(buttons,0); // top level layout as child

  // Setup the `buttons' layout
#if QT_VERSION < 200
  buttons->addStrut(20);
#endif
  buttons->addWidget(instButton,1);
  buttons->addWidget(uinstButton,1);
  buttons->addStretch(1);

  // Activate the layout managers
  vPan->activate(treeList,rightpanel);

  treeList->setMultiSelection(FALSE);
}

int managementWidget::getPSeparator()
{
  return vPan->separatorPos();
}

void managementWidget::setPSeparator(int p)
{
  return vPan->setSeparatorPos(p);
}

void managementWidget::setupInstButton(packageInfo *p)
{
  instButton->disconnect();

  if (p->packageState != packageInfo::INSTALLED &&
      package->getProperty("filename") ) {

    instButton->setEnabled(TRUE);
    uinstButton->setEnabled(FALSE);
    instButton->setText(i18n("Install"));

    disconnect(uinstButton, SIGNAL(clicked()), this,SLOT(uninstallClicked()));
    disconnect(instButton ,SIGNAL(clicked()), this,SLOT(examineClicked()));
    connect(instButton,SIGNAL(clicked()),
	    SLOT(examineClicked()));
  } else {
    instButton->setEnabled(FALSE);
    uinstButton->setEnabled(TRUE);
    uinstButton->setText(i18n("Uninstall"));
    disconnect(uinstButton ,SIGNAL(clicked()), this,SLOT(uninstallClicked()));
    disconnect(instButton ,SIGNAL(clicked()), this,SLOT(examineClicked()));
    connect(uinstButton,SIGNAL(clicked()),
	    SLOT(uninstallClicked()));
  }
}

void managementWidget::arrangeWidgets()
{
  // this is done automatically by the layout managers
}

// Collect data from package.
void managementWidget::collectData(bool refresh)
{
  int i;

  if (!refresh && installedPackages)
    return; // if refresh not required already initialised

  QApplication::setOverrideCursor( waitCursor );

// stop clear() sending selectionChanged signal
  disconnect(treeList, SIGNAL(selectionChanged(QListViewItem *)),
	  this, SLOT(packageHighlighted(QListViewItem *)));
  treeList->hide();    // hide list tree
  treeList->clear();   // empty it 
  connect(treeList, SIGNAL(selectionChanged(QListViewItem *)),
	  SLOT(packageHighlighted(QListViewItem *)));

  packageDisplay->noPackage(); 

  // Delete old list if necessary
  if(installedPackages) {
    delete installedPackages;
  }

  installedPackages = new QList<packageInfo>;
  installedPackages->setAutoDelete(TRUE);

  dirInstPackages->clear();
  dirUninstPackages->clear();
  // List installed packages
  for (i = 0; i < kpinterfaceN; i++)  {
    kpinterface[i]->listPackages(installedPackages);
  }

  // Rebuild the list tree
  rebuildListTree();

  QApplication::restoreOverrideCursor();
}

// Rebuild the list tree
void managementWidget::rebuildListTree()
{
  packageInfo *i;
  int  n = 0;

  kpkg->kp->setStatus(i18n("Building package tree"));
  kpkg->kp->setPercent(0);


  // place all the packages found
  int count = installedPackages->count();

  for(i=installedPackages->first(); i!=0; i=installedPackages->next())
    {
      i->place(treeList);
      
      int num = (n*100)/count;
      if (!(num % 5))
	kpkg->kp->setPercent(num);
      n++;
    }
  treeList->show();		// show the list tree
  setStatus();
  kpkg->kp->setPercent(100);	// set the progress
}

// A package has been highlighted in the list tree
void managementWidget::packageHighlighted(QListViewItem *item)
{

  KTVItem *sel = (KTVItem *)item;

  if (!sel || sel->childCount()) {
    emit changePackage(NULL);
    return;
  }

  if (sel) {
	  // Disable the tree list while we do this
	  treeList->setEnabled(FALSE);
	  // Tell everything that is interested to change packages
	  emit changePackage(sel->info);

	  // Keep copy of the package
	  package = sel->info;
	  if (package)
	    setupInstButton(package);

	  // Re-enable the treeList and uninstall button
	  treeList->setEnabled(TRUE);
  } 

  // Update the status bar
  kpkg->kp->setStatus(i18n("Management Mode: Single Selection"));
  kpkg->kp->setPercent(100);
}

void managementWidget::setStatus()
{
  if (treeList->isMultiSelection()) {
    kpkg->kp->setStatus(i18n("Management Mode: Multiple Selection"));
  } else {
    kpkg->kp->setStatus(i18n("Management Mode: Single Selection"));
  }
}

void managementWidget::toggleSelect()
{
  if (treeList->isMultiSelection()) {
    kpkg->setSelectMode(FALSE);
    //    kpkg->enableMenu();

    clearSelected(treeList->firstChild());

    treeList->setMultiSelection(FALSE);
    
    instButton->setEnabled(FALSE);
    uinstButton->setEnabled(FALSE);
    instButton->setText(i18n("Examine"));

    disconnect(uinstButton,SIGNAL(clicked()),
	       this,SLOT(uninstallMultClicked()));
    disconnect(instButton,SIGNAL(clicked()),
	       this,SLOT(installMultClicked()));
  } else {
    kpkg->setSelectMode(TRUE);
    //    kpkg->disableMenu();

    treeList->setMultiSelection(TRUE);

    instButton->setEnabled(TRUE);
    uinstButton->setEnabled(TRUE);
    instButton->setText(i18n("Install"));

    disconnect(instButton, SIGNAL(clicked()),
	       this,SLOT(examineClicked()));
    disconnect(uinstButton ,SIGNAL(clicked()),
	       this,SLOT(uninstallClicked()));

    connect(uinstButton,SIGNAL(clicked()),
	    this,SLOT(uninstallMultClicked()));
    connect(instButton,SIGNAL(clicked()),
	    this,SLOT(installMultClicked()));
  }
  setStatus();
}


void managementWidget::currentChanged(QListViewItem *p)
{
  KTVItem *sel = (KTVItem *)p;

  if (sel->info && treeList->isMultiSelection() &&
      sel->isSelected() ) {
    if (sel->childCount() == 0)
      emit changePackage(sel->info);
    else
      emit changePackage(0);
  }
}

	   
// exmamine has been clicked
void managementWidget::examineClicked()
{
  if (!package) {
    return;
  }

  if (treeList->isMultiSelection()) { // waddafak
    QString url = package->getUrl();
    if (!url.isEmpty()) {
      kpkg->kp->openNetFile(url);
    } else {
      KpMsgE(i18n("Filename not available\n"),"",TRUE); 
    }
    return;
  } else {
    package->interface->install(0, package);
  }
}

// install has been clicked
void managementWidget::installMultClicked()
{
  int  i;
  KTVItem *it;
  packageInfo *inf;
  QList<packageInfo> **lst = new QList<packageInfo>*[kpinterfaceN];

  selList.clear();
  findSelected(treeList->firstChild());
  for (i = 0; i < kpinterfaceN; i++) {
    lst[i] = new QList<packageInfo>;
    for (it = selList.first(); it != 0; it = selList.next()) {
      if (it->info->interface == kpinterface[i] &&
	  it->childCount() == 0 &&
	  (it->info->packageState == packageInfo::UPDATED ||
	   it->info->packageState == packageInfo::NEW)
	  ) {
	lst[i]->insert(0,it->info);
      }
    }
  }
  selList.clear();
 
  for (i = 0; i < kpinterfaceN; i++) {
    if (lst[i]->count() > 0) {
      kpinterface[i]->installationMult->setup(lst[i],kpinterface[i]->head);
      if (kpinterface[i]->installationMult->exec()) {
	for (inf = lst[i]->first(); inf != 0; inf = lst[i]->next()) {
	  updatePackage(inf,TRUE);
	}
      }
      delete lst[i];
    }
  }
  delete [] lst;
}

// Uninstall has been clicked
void managementWidget::uninstallClicked()
{
  int result;

  if (package)			// check that there is a package to uninstall
    {
      package->interface->uninstallation->setup(package);
      result = package->interface->uninstallation->exec();

      if(result == QDialog::Accepted) // execute it
	{			// it was accepted, so the package has been
				// uninstalled
	  updatePackage(package,FALSE);

	  emit changePackage(NULL); // change package to no package
	  package = NULL;
	  instButton->setEnabled(FALSE); // disable uninstall button
	}
    }
  setStatus();
  kpkg->kp->setPercent(100);
}

// Uninstall multiple  has been clicked
void managementWidget::uninstallMultClicked()
{
  int  i;
  KTVItem *it;
  packageInfo *inf;
  QList<packageInfo> **lst = new QList<packageInfo>*[kpinterfaceN];

  selList.clear();
  findSelected(treeList->firstChild());
  for (i = 0; i < kpinterfaceN; i++) {
    lst[i] = new QList<packageInfo>;
    for (it = selList.first(); it != 0; it = selList.next()) {
      if (it->info->interface == kpinterface[i] &&
	  it->childCount() == 0 &&
	  (it->info->packageState == packageInfo::INSTALLED ||
	   it->info->packageState == packageInfo::BAD_INSTALL)
	  ) {
	lst[i]->insert(0,it->info);
      }
    }
  }
  selList.clear();
 
  for (i = 0; i < kpinterfaceN; i++) {
    if (lst[i]->count() > 0) {
      kpinterface[i]->uninstallationMult->setup(lst[i],kpinterface[i]->head);
      if (kpinterface[i]->uninstallationMult->exec()) {
	for (inf = lst[i]->first(); inf != 0; inf = lst[i]->next()) {
	  updatePackage(inf,FALSE);
	}
	delete lst[i];
      }
    }
  }
  delete [] lst;
}

///////////////////////////////////////////////////////////////////////////
QListViewItem *managementWidget::search(const char *str, const char* head,
				QListViewItem  *start) 
{
  QListViewItem  *item = treeList->firstChild();

  searchCitem = start;
  searchSkip = FALSE;
  searchSubstr = FALSE;
  searchStr = str;
  searchResult = 0;

  do {
    if (!strcmp(item->text(0),head)) {
      searchChild(item->firstChild());
      if (searchResult != 0) 
	return searchResult;
    }
  } while ((item = item->nextSibling()));
  return 0;
}

QListViewItem *managementWidget::search(const char *str, bool subStr, bool wrap,
			     bool start=FALSE)
{
  if (!treeList->firstChild())
    return 0;

  if (start)
    searchCitem = 0;
  else
    searchCitem = treeList->currentItem();
  searchSkip = !wrap;
  searchSubstr = subStr;
  searchStr = str;
  searchResult = 0;

  searchChild(treeList->firstChild());
  
  if (searchResult) {
    	QListViewItem *i;

	i = searchResult;
	while ((i = i->parent())) {
	  i->setOpen(TRUE);	
	}
	treeList->setSelected(searchResult,TRUE);
	treeList->setCurrentItem(searchResult);
	treeList->ensureItemVisible(searchResult);
	return searchResult;
  } else {
    return 0;
  }
}

bool managementWidget::searchChild(QListViewItem *it)
{
  do {
    if (!searchSkip) {
      QString s = it->text(0);
      if ((it->childCount() == 0) &&
	  (searchSubstr ? s.contains(searchStr,FALSE) : s == searchStr)) {
	searchResult = it;
	return TRUE;
      }
    }

    if (searchCitem == it) {
      if (searchSkip) {
	searchSkip = FALSE;
      } else {
	return TRUE;
      }
    }

    if (it->childCount() > 0) {
      if (searchChild(it->firstChild()))
	return TRUE;
    }
  } while ((it = (KTVItem *)it->nextSibling()));
  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
QListViewItem *managementWidget::updatePackage(packageInfo *pki, bool install)
{
  QString version;

  if (installedPackages) {
    QString name = pki->getProperty("name")->data();
    if (pki->getProperty("version"))
      version = pki->getProperty("version")->data();
    else
      version = "";
    pkgInterface *interface = pki->interface;
    packageInfo *pnew = interface->getPackageInfo('i', name.data(), version.data());
    packageInfo *ptree;
    QString dirIndex =  name + interface->typeID;
    //printf("name=%s\n",name.data());
    if (install) {
      if (pnew) {
	//printf("pnew=%s\n",pnew->getProperty("name")->data());
	if (pnew->packageState !=  packageInfo::BAD_INSTALL) {
	  ptree = dirInstPackages->find(dirIndex); // remove installed entry
	  dirInstPackages->remove(dirIndex);
	  if (ptree) {
	    if (ptree->getItem()) 
	      delete ptree->getItem();
	  }

	  ptree = dirUninstPackages->find(dirIndex); // remove uninstalled entry
	  dirUninstPackages->remove(dirIndex);
	  if (ptree) {
	    //printf("ptree=%s\n",ptree->getProperty("name")->data());
	    if (ptree->getItem()) 
	      delete ptree->getItem();	
	  }
	}

	dirInstPackages->insert(dirIndex,pnew); 

	QListViewItem *q = pnew->place(treeList,TRUE);
	installedPackages->insert(0,pnew);
	if (!q) {
	  printf("NOTP=%s \n",pnew->getProperty("name")->data());
	} else {
	  return q;
	}
      }
    } else {
      if (!pnew) {
	dirInstPackages->remove(dirIndex);
	if (pki->getItem()) {
	  delete pki->getItem();
	} else {
	  printf("DEL=%s\n",name.data());
	}
      } else {
	delete pnew;
      }
    }
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////
void managementWidget::clearSelected(QListViewItem *item)
{
  do {
    if (item->childCount() > 0) {
      clearSelected(item->firstChild());
    }
    if (item->isSelected()) {
      treeList->setSelected(item,FALSE);
    }
  } while ((item = (KTVItem *)item->nextSibling()));
}
  
void managementWidget::findSelected(QListViewItem *item)
{
  do {
    if (item->childCount() > 0) {
      findSelected(item->firstChild());
    }
    if (item->isSelected()) {
      selList.insert(0,(KTVItem *)item);
    }
  } while ((item = (KTVItem *)item->nextSibling()));
}
  
///////////////////////////////////////////////////////////////////////////
void managementWidget::expandTree(QListView *list)
{
  QListViewItem *item = list->firstChild();

  do {
    if (item->childCount() > 0) {
      item->setOpen(TRUE);
      expandTree(item);
    }
  } while ((item = item->nextSibling()));
}
  

void managementWidget::expandTree(QListViewItem *pitem)
{
  QListViewItem *item = pitem->firstChild();
    
  do {
    if (item->childCount() > 0) {
      item->setOpen(TRUE);
      expandTree(item);
    }
  } while ((item = item->nextSibling()));
}

void managementWidget::collapseTree(QListView *list)
{
  QListViewItem *item = list->firstChild();

  do {
    if (item->childCount() > 0) {
      collapseTree(item);
    }
  } while ((item = item->nextSibling()));
}

void managementWidget::collapseTree(QListViewItem *pitem)
{
  int n = 0;
  QListViewItem *item = pitem->firstChild();

  do {
    if (item->childCount() > 0) {
      n++;
      collapseTree(item);
    }
  } while ((item = item->nextSibling()));
  if (n)
    pitem->setOpen(TRUE);
  else
    pitem->setOpen(FALSE);
}
  
///////////////////////////////////////////////////////////////////////////
void managementWidget::writeTreePos()
{
  int i;

  KConfig *config = app->getConfig();

  config->setGroup("Treelist");

  QString colpos;
  for (i = 0; i < 4; i++) {
    colpos.setNum(i);
    config->writeEntry(colpos,treeList->columnWidth(i));
  }
}

void managementWidget::readTreePos()
{
  int i, n;
  int num[] = {185,60,95,95};

  KConfig *config = app->getConfig();

  config->setGroup("Treelist");

  QString colpos;
  for (i = 0; i < 4; i++) {
    colpos.setNum(i);
    n = config->readNumEntry(colpos.data(),num[i]);
    treeList->setColumnWidth(i,n);
  }
}


