//////////////////////////////////////////////////////////////////////
//      $Id: kpackage.cpp 21066 1999-05-08 19:49:53Z bieker $ 
// File  : kpackage.cpp
// Author: Damyan Pepper
//         Toivo Pedaste
//
// See kpackage.h for more information.
//////////////////////////////////////////////////////////////////////

#include "config.h"
extern "C"
{
#include <time.h>
}  
#include <unistd.h>

#include <qdir.h>
#include <kurl.h>
#include <kapp.h>
#include <kaccel.h>
#include <kkeydialog.h>
#include <klocale.h>
 
#if QT_VERSION >= 200
#include <kaccelmenu.h>
#endif

#include "kpackage.h"
#include "pkgInterface.h"
#include "aboutDialog.h"
#include "managementWidget.h"
#include "installationWidget.h"
#include "pkguninstallDialog.h"
#include "pkgInterface.h"
#if QT_VERSION >= 200
#include "kio.h"
#endif
#include "findf.h"
#include "search.h"
#include "options.h"
#include "cache.h"

extern KApplication *app;

extern Params *params;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
KPKG::KPKG(KConfig *_config, const char *name) 
  : KTopLevelWidget(name)
{
  hide_toolbar=FALSE;

  kp = new KPACKAGE(_config,this);
  kp->show();
  setView(kp);

  readSettings();

  // Get a nice default size
  resize(600,400);

  setupMenu();
  setupToolBar();

  optiondialog = new Options(0, "optionsdialog");
  recent_files.setAutoDelete(TRUE);

  recentpopup->clear();

  for ( int i =0 ; i < (int)recent_files.count(); i++){
    recentpopup->insertItem(recent_files.at(i));
  }
  prop_restart = false;

  config->setGroup("Kpackage");

  int width, height;
  width = config->readNumEntry("Width");
  height = config->readNumEntry("Height");

  if( width < minimumSize().width() ) {
    width = minimumSize().width();
  }
  if (!width)
    width=600;
  if( height < minimumSize().height() ) {
    height = minimumSize().height();
  }
  if (!height)
    height=400;

  resize(width, height);

}

// Destructor
KPKG::~KPKG()
{
  //   object deletion - slows down exit
  //    delete filemenu;
  //    delete helpmenu;
  //    delete recentpopup;
  //    delete kp; 
  //    delete options; 
  //    delete optiondialog; 
  //    delete caches;
}

// Set up the menu

#if QT_VERSION >= 200

void KPKG::setupMenu()
{

  keys = new KAccel(this);

  recentpopup = new QPopupMenu ();

  filemenu = new KAccelMenu(keys);
  filemenu->insertSeparator();

  filemenu->insItem(i18n("&Open"), "&Open", kp ,SLOT(fileOpen()), KAccel::Open);
  filemenu->insItem(i18n("Open &URL"), "Open &URL", kp, SLOT(fileOpenUrl()),"CTRL+X");

  filemenu->insertItem (i18n("Open &Recent..."), recentpopup);
  connect( recentpopup, SIGNAL(activated(int)), SLOT(openRecent(int)) );

  filemenu->insItem(i18n("Find &Package..."),"Find &Package...",
		    kp, SLOT(find()), KAccel::Find);
  filemenu->insItem(i18n("Find &File..."), "Find &File...", kp, SLOT(findf()));
  filemenu->insItem(i18n("Re&load"), "Re&load", kp, SLOT(reload()));
  filemenu->insItem(i18n("&Quit"), "&Quit", kp, SLOT(fileQuit()), KAccel::Quit);

  packmenu = new KAccelMenu(keys);
  packmenu->insItem(i18n("&Expand Tree"), "&Expand Tree", kp, SLOT(expandTree()));
  packmenu->insItem(i18n("&Collapse Tree"), "&Collapse Tree", kp, SLOT(collapseTree()));
  packmenu->insItem(i18n("Clear &Selected"), "Clear &Selected", kp, SLOT(clearSelected()));
  packmenu->insertSeparator();
  selectID = packmenu->insItem(i18n("Multiple Selection &Mode"),
			       "Multiple Selection &Mode", kp, SLOT(toggleSelect()));
  
  options = 	new KAccelMenu (keys);
  options->insItem(i18n("&Options..."), "&Options...",
		      this, 	SLOT(setOptions()));
  options->insItem(i18n("&Keys..."), "&Keys...",
		      this, 	SLOT(setKeys()));

  locationpopup = new KAccelMenu(keys);
  options->insertItem (i18n("&Location of uninstalled packages"), locationpopup);
  int i;
  QString iloc, loc;
  for (i = 0; i < kpinterfaceN; i++) {
    if (kpinterface[i]->locatedialog) {
      iloc = i18n("Location ");
      loc = "Location ";
      iloc += "&";
      loc += "&";
      iloc += kpinterface[i]->head;
      loc += kpinterface[i]->head;
      iloc += "...";
      loc += "...";
      locationpopup->insItem(iloc, strdup(loc),  kpinterface[i],
		       SLOT(setLocation()));
    }
  }

  options->insertSeparator(-1);
  if(hide_toolbar) 
    toolID   = options->insItem(i18n("Show &Tool Bar"), "Show &Tool Bar",
				 this,SLOT(toggleToolBar()));
  else
    toolID   = options->insItem(i18n("Hide &Tool Bar"), "Hide &Tool Bar",
				 this,SLOT(toggleToolBar()));
  options->insertSeparator(-1);
  options->insItem(i18n("&Save Settings Now"), "&Save Settings Now",
		      this, 	SLOT(saveSettings()));

  caches = 	new KAccelMenu (keys);
  caches->insItem(i18n("Clear package &Directory cache"),
		  "Clear package &Directory cache", this, SLOT(clearDCache()));
  caches->insItem(i18n("Clear &Package cache"), "Clear &Package cache",
		      this, 	SLOT(clearPCache()));

  helpmenu = new KAccelMenu(keys);
  helpmenu->insItem(i18n("&Contents"), "&Contents", kp, SLOT(helpHelp()), KAccel::Help);
  helpmenu->insertSeparator(-1);
  helpmenu->insItem(i18n("&About..."), "&About...", kp, SLOT(helpAbout()));


  menubar = new KMenuBar(this);
  menubar->insertItem(i18n("&File"),filemenu);
  menubar->insertItem(i18n("&Packages"),packmenu);
  menubar->insertItem(i18n("&Options"),options);
  menubar->insertItem(i18n("&Cache"),caches);
  menubar->insertSeparator();
  menubar->insertItem(i18n("&Help"),helpmenu);
 
  config->setGroup("Kpackage");
  menubar->setMenuBarPos( (KMenuBar::menuPosition) config->readNumEntry("MenuBarPos") );
  setMenu(menubar);

  keys->readSettings();

  urlList.setAutoDelete(TRUE);

}

#else

void KPKG::setupMenu()
{

  recentpopup = new QPopupMenu ();

  filemenu = new QPopupMenu ();
  filemenu->insertSeparator();

  filemenu->insertItem(i18n("&Open"), kp ,SLOT(fileOpen()));
  filemenu->insertItem(i18n("Open &URL"), kp, SLOT(fileOpenUrl()));

  filemenu->insertItem (i18n("Open &Recent..."), recentpopup);
  connect( recentpopup, SIGNAL(activated(int)), SLOT(openRecent(int)) );

  filemenu->insertItem(i18n("Find &Package..."),  kp, SLOT(find()));
  filemenu->insertItem(i18n("Find &File..."), kp, SLOT(findf()));
  filemenu->insertItem(i18n("Re&load"), kp, SLOT(reload()));
  filemenu->insertItem(i18n("&Quit"), kp, SLOT(fileQuit()));

  packmenu = new QPopupMenu ();
  packmenu->insertItem(i18n("&Expand Tree"), kp, SLOT(expandTree()));
  packmenu->insertItem(i18n("&Collapse Tree"), kp, SLOT(collapseTree()));
  packmenu->insertItem(i18n("Clear &Selected"), kp, SLOT(clearSelected()));
  packmenu->insertSeparator();
  selectID = packmenu->insertItem(i18n("Multiple Selection &Mode"),
kp, SLOT(toggleSelect()));
  
  options = 	new QPopupMenu ();
  options->insertItem(i18n("&Options..."),   this, 	SLOT(setOptions()));

  locationpopup = new  QPopupMenu ();
  options->insertItem (i18n("&Location of uninstalled packages"), locationpopup);
  int i;
  QString iloc, loc;
  for (i = 0; i < kpinterfaceN; i++) {
    if (kpinterface[i]->locatedialog) {
      iloc = i18n("Location ");
      loc = "Location ";
      iloc += "&";
      loc += "&";
      iloc += kpinterface[i]->head;
      loc += kpinterface[i]->head;
      iloc += "...";
      loc += "...";
      locationpopup->insertItem(iloc,  kpinterface[i],
		       SLOT(setLocation()));
    }
  }

  options->insertSeparator(-1);
  if(hide_toolbar) 
    toolID   = options->insertItem(i18n("Show &Tool Bar"), 					 this,SLOT(toggleToolBar()));
  else
    toolID   = options->insertItem(i18n("Hide &Tool Bar"), 					 this,SLOT(toggleToolBar()));
  options->insertSeparator(-1);
  options->insertItem(i18n("&Save Settings Now"),    this, 	SLOT(saveSettings()));

  caches = 	new  QPopupMenu ();
  caches->insertItem(i18n("Clear package &Directory cache"),
		     this, SLOT(clearDCache()));
  caches->insertItem(i18n("Clear &Package cache"),    this, 	SLOT(clearPCache()));

  helpmenu = new  QPopupMenu ();
  helpmenu->insertItem(i18n("&Contents"), kp, SLOT(helpHelp()));
  helpmenu->insertSeparator(-1);
  helpmenu->insertItem(i18n("&About..."), kp, SLOT(helpAbout()));


  menubar = new KMenuBar(this);
  menubar->insertItem(i18n("&File"),filemenu);
  menubar->insertItem(i18n("&Packages"),packmenu);
  menubar->insertItem(i18n("&Options"),options);
  menubar->insertItem(i18n("&Cache"),caches);
  menubar->insertSeparator();
  menubar->insertItem(i18n("&Help"),helpmenu);
 
  config->setGroup("Kpackage");
  menubar->setMenuBarPos( (KMenuBar::menuPosition) config->readNumEntry("MenuBarPos") );
  setMenu(menubar);

  urlList.setAutoDelete(TRUE);

}

#endif

void KPKG::setupToolBar()
{
  toolbar = new KToolBar( this );

  KIconLoader *loader = kapp->getIconLoader();
  QPixmap pixmap;

  toolbar->insertButton(loader->loadIcon("fileopen.xpm"), 0,
		      SIGNAL(clicked()), kp,
		      SLOT(fileOpen()), TRUE, i18n("Open Package"));

  toolbar->insertButton(loader->loadIcon("ftout.xpm"), 0,
		      SIGNAL(clicked()), kp,
		      SLOT(expandTree()), TRUE, i18n("Expand Package Tree"));

  toolbar->insertButton(loader->loadIcon("ftin.xpm"), 0,
		      SIGNAL(clicked()), kp,
		      SLOT(collapseTree()), TRUE, i18n("Collapse Package Tree"));

  toolbar->insertButton(loader->loadIcon("find.xpm"), 0,
		      SIGNAL(clicked()), kp,
		      SLOT(find()), TRUE, i18n("Find Installed Package"));

  toolbar->insertButton(loader->loadIcon("findf.xpm"), 0,
		      SIGNAL(clicked()), kp,
		      SLOT(findf()), TRUE, i18n("Find Package containing file"));

  toolbar->insertButton(loader->loadIcon("reload.xpm"), 0,
		      SIGNAL(clicked()), kp,
		      SLOT(reload()), TRUE, i18n("Reload Package Information"));

  config->setGroup("Kpackage");

  toolbar->setBarPos( (KToolBar::BarPosition) config->readNumEntry("ToolBarPos") );
  toolbar1 = addToolBar(toolbar);

  QString str = config->readEntry( "ToolBar" );
  if ( !str.isNull() && str.find( "off" ) == 0 ) {
    hide_toolbar = TRUE;
    enableToolBar( KToolBar::Hide, toolbar1 );
  } else{
    hide_toolbar = FALSE;
  }
}

void KPKG::disableMenu()
{
 filemenu->setItemEnabled(filemenu->idAt(1),FALSE);
 filemenu->setItemEnabled(filemenu->idAt(2),FALSE);
 filemenu->setItemEnabled(filemenu->idAt(3),FALSE);
 filemenu->setItemEnabled(filemenu->idAt(4),FALSE);
 toolbar->setItemEnabled(0,FALSE);
}

void KPKG::enableMenu()
{
 filemenu->setItemEnabled(filemenu->idAt(1),TRUE);
 filemenu->setItemEnabled(filemenu->idAt(2),TRUE);
 filemenu->setItemEnabled(filemenu->idAt(3),TRUE);
 filemenu->setItemEnabled(filemenu->idAt(4),TRUE);
 toolbar->setItemEnabled(0,TRUE);
}

void KPKG::openRecent(int i){
  kp->openNetFile( recent_files.at(recentpopup->indexOf(i)) );	  
}

void KPKG::add_recent_file(const char* newfile){
  if(recent_files.find(newfile) != -1)
    return; // it's already there

  if( recent_files.count() < 8)
    recent_files.insert(0,newfile);
  else{
    recent_files.remove(7);
    recent_files.insert(0,newfile);
  }

  recentpopup->clear();

  for ( int i =0 ; i < (int)recent_files.count(); i++){
    recentpopup->insertItem(recent_files.at(i));
  }
}

void KPKG::readSettings() {
  QString str;
  int i;

  config = app->getConfig();

  config->setGroup("Kpackage");
  kp->management->setPSeparator
    (config->readNumEntry("Panel_Separator",260));

  config->setGroup("Recently_Opened_Files");

  QString recent_number;
  for (i = 0; i < 8; i++) {
    recent_number.setNum(i+1);
    str = config->readEntry(recent_number,"");
    if (!str.isEmpty())
      recent_files.append(str.data());
  }
}

void KPKG::toggleToolBar() {
  if(hide_toolbar) {
    hide_toolbar=FALSE;
    enableToolBar( KToolBar::Show, toolbar1 );
    options->changeItem(i18n("Hide &Tool Bar"), toolID);
  } 
  else {
    hide_toolbar=TRUE;
    enableToolBar( KToolBar::Hide, toolbar1 );
    options->changeItem(i18n("Show &Tool Bar"), toolID);
  }  
}	

void KPKG::setSelectMode(bool mult) {
  if(mult) {
    packmenu->changeItem(i18n("Single Selection &Mode"), selectID);
  } else {
    packmenu->changeItem(i18n("Multiple Selection &Mode"), selectID);
  }  
}	

void KPKG::writeSettings(){
  int i;

  KConfig *config = app->getConfig();

  config->setGroup("Kpackage");

  config->writeEntry("Width", width());
  config->writeEntry("Height", height());
  config->writeEntry( "ToolBar", hide_toolbar ? "off" : "on" );

  // Bars created floating don't work
  if (toolbar->barPos() == KToolBar::Floating)
    config->writeEntry("ToolBarPos", (int)KToolBar::Top );
  else
    config->writeEntry("ToolBarPos", (int) toolbar->barPos() );
  if (menubar->menuBarPos() == KMenuBar::Floating)
    config->writeEntry("MenuBarPos", (int) KMenuBar::Top );
  else
    config->writeEntry("MenuBarPos", (int) menubar->menuBarPos() );
  config->writeEntry("Panel_Separator",
		     kp->management->getPSeparator());

  config->setGroup("Recently_Opened_Files");

  QString recent_number;
  for(i = 0; i <(int) recent_files.count();i++){
    recent_number.setNum(i+1);
    config->writeEntry(recent_number,recent_files.at(i));
  }	

  kp->management->writeTreePos();

#if QT_VERSION >= 200
  keys->writeSettings();
#endif

  config->sync();
}

void KPKG::setOptions(){
  optiondialog->restore();
}

void KPKG::setKeys(){
#if QT_VERSION >= 200
  KKeyDialog::configureKeys( keys ); 
#endif
}

void KPKG::saveSettings(){
  writeSettings();
}

void KPKG::clearPCache(){
  cacheObj::clearPCache();
}

void KPKG::clearDCache(){
  cacheObj::clearDCache();
}

void KPKG::saveProperties(KConfig *config )
{
    config->writeEntry("Name", kp->save_url);
}


void KPKG::readProperties(KConfig *config)
{
    QString entry = config->readEntry("Name"); // no default
    if (entry.isNull())
	return;
    kp->openNetFile(entry.data());
    prop_restart = true;
}

void KPKG::closeEvent ( QCloseEvent *) {
    kp->fileQuit();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
KPACKAGE::KPACKAGE(KConfig *_config, QWidget *parent, const char *name) 
  : QWidget(parent,name)
{

  // Save copy of config
  config = _config;

  // Setup the dropzone
  setupDropzone();
  // Setup the mode widgets
  setupModeWidgets();

  // Setup the status bar
  setupStatusBar();

  file_dialog = NULL;
  findialog = NULL;
  srchdialog = NULL;  
}

// Destructor
KPACKAGE::~KPACKAGE()
{
  destroyModeWidgets();
  delete status;
  delete processProgress;
}

// resize event -- arrange the widgets
void KPACKAGE::resizeEvent(QResizeEvent *re)
{
  re = re;			// prevent warning
  arrangeWidgets();
}

// Set up the drop zone
void KPACKAGE::setupDropzone()
{
  dropzone = new KDNDDropZone(this, DndURL);
  connect(dropzone,SIGNAL(dropAction(KDNDDropZone*)),
	  SLOT(dropAction(KDNDDropZone*)));
}

// Set up the mode widgets
void KPACKAGE::setupModeWidgets()
{
  management = new managementWidget(this);
  for (int i = 0; i < kpinterfaceN; i++) {

    kpinterface[i]->installation =
      new installationWidget(this, kpinterface[i]);
    connect(kpinterface[i]->installation,
	    SIGNAL(finished(int, pkgInterface *,int)),
	    SLOT(modeFinished(int, pkgInterface *,int)));

    kpinterface[i]->uninstallation =
      new pkguninstallDialog(kpinterface[i]->inituninstallOptions());
    kpinterface[i]->uninstallationMult =
      new pkguninstallDialogMult(kpinterface[i]->inituninstallOptions());
    kpinterface[i]->installationMult =
      new pkginstallDialogMult(kpinterface[i]->initinstallOptions());
  }
}

// destroy the mode widgets
void KPACKAGE::destroyModeWidgets()
{
  delete management;
  for (int i = 0; i < kpinterfaceN; i++) {
    delete kpinterface[i]->installation;
    delete kpinterface[i]->uninstallation;
  }
}


// Set up the status bar
void KPACKAGE::setupStatusBar()
{
  statusbar = new QFrame(this);
  statusbar->setFrameStyle(QFrame::Raised | QFrame::Panel);
  processProgress = new KProgress(0,100,100,KProgress::Horizontal,statusbar);
  processProgress->setBarStyle(KProgress::Solid);
  processProgress->setTextEnabled(FALSE);

  status = new QLabel(i18n("Management Mode"), statusbar);
  status->setFont(QFont("Helvetica",10, QFont::Normal));
}
  
// Arrange the widgets nicely
void KPACKAGE::arrangeWidgets()
{
  int i;

  statusbar->resize(width(),20);
  statusbar->move(0,height()-20);
  status->resize((statusbar->width() / 4) * 3, 16);
  status->move(2,2);
  processProgress->resize(statusbar->width() / 4 - 4, 16);
  processProgress->move((statusbar->width() / 4) * 3 + 3, 2);
  
  management->resize(width(),height() - 20);

  for (i = 0; i < kpinterfaceN; i++) 
    kpinterface[i]->installation->resize(width(),height() - 20);
}

// Change the mode
void KPACKAGE::setMode(int newmode, pkgInterface *type, int refresh) 
{ 
  int i;

  if(newmode == Management)	// entering management mode so...
    {
      for (i = 0; i < kpinterfaceN; i++)
	kpinterface[i]->installation->hide();
      management->show();	// show the management widget
      kpkg->enableMenu();
      management->collectData(refresh); // refresh the management widget
    }
  else				// entering installation mode so...
    {
      for (i = 0; i < kpinterfaceN; i++) {// hide all except the 
	if (kpinterface[i] == type)       // correct install widget
	  kpinterface[i]->installation->show();
	else
	  kpinterface[i]->installation->hide();
      }
      kpkg->disableMenu();
      
      management->hide();	// hide the management widget
    }
}

void KPACKAGE::fileQuit()		// file->quit selected from menu
{
  kpkg->writeSettings();
  if (params->DCache >= Params::SESSION) {
    cacheObj::clearDCache(); // clear dir caches if needed
  }
  if (params->PCache >= Params::SESSION) {
    cacheObj::clearPCache(); // clear package caches if needed
  }

  KApplication::exit(0);	// exit the application
}

void KPACKAGE::reload()
{
  kpkg->kp->management->collectData(TRUE);
}

void KPACKAGE::fileOpen()		// file->quit selected from menu
{
    QString fname;
    KFileDialog *box;
    //    pkgInterface *type;

    box = getFileDialog(i18n("Select Document to Open"));
    
    box->show();
    
    if (!box->result())   /* cancelled */
      return;
    if(box->selectedFileURL().isEmpty()) {  /* no selection */
      return;
    }
    

    fname =  box->selectedFileURL();
    openNetFile(fname);
}

void KPACKAGE::clearSelected()
{
  management->clearSelected(management->treeList->firstChild());
}

void KPACKAGE::expandTree()
{
  management->expandTree(management->treeList);
}

void KPACKAGE::collapseTree()
{
  management->collapseTree(management->treeList);
}

void KPACKAGE::toggleSelect()
{
  management->toggleSelect();
}

pkgInterface *KPACKAGE::pkType(const char *fname)
{
  // Get the package information for this package
  char buf[50];
  int i;

  FILE *file= fopen(fname,"r");
  if (file) {
    fgets(buf,sizeof(buf),file);

    for (i = 0; i < kpinterfaceN; i++) {
      if (kpinterface[i]->isType(buf, fname)) {
	fclose(file);
	return kpinterface[i];
      }
    }
    fclose(file);
    KpMsgE(i18n("Unknown package type: %s"),fname,TRUE);
  } else {
    KpMsgE(i18n("File not found: %s"),fname,TRUE);
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////

void KPACKAGE::openNetFile( const char *_url )
{
  QString s = fetchNetFile(_url);
  kpkg->add_recent_file(_url);
  if (!s.isEmpty()) {
    pkgInterface *type = pkType(s.data());
    if (type) {
      setMode(Installation,type,0);	// enter installation mode
      installPackage(s.data(),type);	// start installing this package
    }
  }
}

QString KPACKAGE::getFileName(QString url, QString &cacheName )
{
  QString none  = "";
  QString fname = "";

  KURL *u = new KURL( url.data() );
  if ( u->isMalformed() )  {
    delete u; 

    if (url.data()[0] == '/') {	// absolute path
      u=new KURL( QString(QString("file:")+QString(url.data())) );
    } else {
      u=new KURL( QString(QString("file:")+
			  QDir::currentDirPath()+"/"+QString(url.data())) );
    }
  }
  if (u->isMalformed()) {
    KpMsgE(i18n("Malformed URL: %s"),url.data(),TRUE);
  } else {

    // Just a usual file ?
    if ( strcmp( u->protocol(), "file" ) == 0 ) {
      cacheName = u->path();
      fname = u->path();
    } else {
    
      QString tmpd = cacheObj::PDir();
      if (!tmpd.isEmpty()) {

	QString cacheFile;
	QString file = u->path();
	int pt = file.findRev('/');
	pt++;
	if (pt >= 0) {  			// file name for cache
	  cacheFile = tmpd + file.right(file.length() - pt);
	} else {
	  cacheFile = tmpd + file;
	}

	cacheName = cacheFile.data();
	QFileInfo f(cacheFile.data());
	if (f.exists() && (params->DCache != Params::NEVER)) {
	  fname =  cacheFile;
	}
      }
    }
  }
  delete u;
  return fname;
}
     
QString KPACKAGE::fetchNetFile( QString url )
{

  QString cf;

  QString f = getFileName(url, cf);

  if (cf.isEmpty()) {
    return "";
  } else {

    if (!f.isEmpty()) {
      return f;
    } else {
      save_url = url;

      setStatus(i18n("Calling KFM"));
  
#if QT_VERSION < 200
      if (KFM::download(url, cf)) {
#else
      Kio kio;

      if (kio.download(url, cf)) {
#endif
	setStatus(i18n("KFM finished"));
	QFileInfo f(cf.data());
	if (!(f.exists() && f.size() > 0)) {
	  unlink(cf.data());
	  return "";
	} else {
	  return cf;
	}
      } else {
	setStatus(i18n("KFM failed"));
	return "";
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////
void KPACKAGE::fileOpenUrl(){

  DlgLocation geturl( i18n("Open Location:"), save_url.data(), this );

  if ( geturl.exec() )
    {
      kpkg->add_recent_file(geturl.getText());
      openNetFile( geturl.getText() );
    }
}

void KPACKAGE::find(){
  if (srchdialog)
    srchdialog->show();
  else
    srchdialog = new Search(0, "find package");
}

void KPACKAGE::findf(){
  if (findialog)
    findialog->show();
  else
    findialog = new FindF(0, "find ffile");
}

KFileDialog* KPACKAGE::getFileDialog(const char* captiontext){

  if(!file_dialog) {
    QString pat;
    for (int i = 0; i < kpinterfaceN; i++) {
      pat += kpinterface[i]->packagePattern;
      pat += " ";
    }
    file_dialog = new KFileDialog(QDir::currentDirPath(), pat,
				  this,"file_dialog",TRUE);
  }

  file_dialog->setCaption(captiontext);
  file_dialog->rereadDir();

  return file_dialog;
}

void KPACKAGE::helpAbout()	// help->about selected from menu
{
  aboutDialog *about = new aboutDialog(this); // create the about box
  about->exec();		// execute the about box
  delete about;			// delete the about box
}

void KPACKAGE::helpHelp()		// Help->Help selected from menu
{
      app->invokeHTMLHelp( "kpackage/index.html", "" );  
}

void KPACKAGE::dropAction(KDNDDropZone *dz) // something has been dropped
{
  char *s;
  int i = 0;

  // Call openNetFile with the first URL, save the rest to be processed
  QStrList &list = dz->getURLList();
  kpkg->urlList.clear();
  for ( s = list.first();
	s != 0;
	s = list.next(), i++ ) {
    if (i == 0)
      openNetFile(s);
    else
      kpkg->urlList.append(s);
  }
}

void KPACKAGE::installPackage(const char *name, pkgInterface *type)	// start installing package
{
    type->installation->installPackage(name); // so tell the installation widget about it
}

void KPACKAGE::modeFinished(int mode, pkgInterface *interface, int refresh) // a mode has finished
{				// so set the new mode appropriately
  if(mode == Installation)
    setMode(Management, interface, refresh);
  else
    setMode(Installation, interface, refresh);
}

void KPACKAGE::setStatus(const char *s)	// set the text in the status bar
{
  status->setText(s);
  kapp->processEvents();	// refresh the screen
}

QString KPACKAGE::getStatus()	// get the text in the status bar
{
  if(status)
    return status->text();
  else 
    return "";
}

void KPACKAGE::setPercent(int x)	// set the progress in the status bar
{
  processProgress->setValue(x);
  kapp->processEvents();	// refresh it
}

//////////////////////////////////////////////////////////////////////////////


