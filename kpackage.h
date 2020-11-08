
//////////////////////////////////////////////////////////////////////////
// $Id: kpackage.h 20671 1999-05-02 14:28:37Z coolo $	
// File  : kpackage.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This is the main widget for kpackage
// The whole widget is a DND drop zone where users can drop packagess to 
// be installed.
// The widget can be in one of two modes: management and installation.
//
// Management mode displays a tree list showing all the packages that are 
// currently installed on the system (each in their appropriate groups)
// and a display of the currently selected package's properties.
// 
// Installation mode displays the properties of the package to install
// and a set of options for installation
//
// The displays for each of these modes are contained within a widget.
// When the mode changes, the appropriate widget is show and the other
// hidden.
//
// KPACKAGE also holds the menu bar.  At current, the menu structure is as
// follows:kpackage
//
// File -> Quit
// Help -> About
//
// This will, of course, be expanded in future versions as more 
// functionality is added to the application.
//////////////////////////////////////////////////////////////////////////

#ifndef KPACKAGE_H
#define KPACKAGE_H

#include "../config.h"

// Standard Headers
#include <stdio.h>

// Qt Headers
#include <qdir.h>
#include <qwidget.h>
#include <qframe.h>
#include <qlabel.h>
#include <qfiledlg.h> 
#include <qgrpbox.h> 

// KDE headers
#include <kfm.h>
#include <kapp.h>
#include <kurl.h>
#include <drag.h>
#include <kprogress.h>
#include <kmenubar.h>
#include <ktopwidget.h> 
#include <kfiledialog.h> 

class Search;
class FindF;
class Options;
class pkgInterface;
class managementWidget;
class KAccelMenu;
class KAccel;


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class KPACKAGE : public QWidget
{
  Q_OBJECT;

  ///////////// METHODS ------------------------------------------------------
public:
  KPACKAGE(KConfig *_config, QWidget *parent=0, const char *name=0);
  // Constructor

  ~KPACKAGE();
  // Destructor

    enum { OPEN_READWRITE 	= 1, 
	   OPEN_READONLY 	= 2, 
	   OPEN_INSERT 		= 4 };

  void setStatus(const char *s);
  // this sets the status bar's string to s

  void setPercent(int x);
  // this set the status bar's progress to x

  QString getStatus();
  // this gets the current status string on the status bar

  void setMode(int newmode, pkgInterface *type, int refresh);
  // This sets the mode to newmode and updates the display accordingly.

  void installPackage(const char *name, pkgInterface *type);
  // this installs a package

  pkgInterface *pkType(const char *fname);
  // find type of package

  void openNetFile(const char *);
  // open a file given a URL

  QString fetchNetFile(QString url);
  // fetch a file given a URL

  static QString getFileName(QString url, QString &cacheName);
  // return file name, if not local file cachename is name for cache entry


protected:
  void resizeEvent(QResizeEvent *re);
  // This is called when the widget is resized


private:

  void setupDropzone();
  // This sets up the DND drop zone

  void setupModeWidgets();
  // This sets up the mode widgets (ie management/installation widgets)

  void destroyModeWidgets();
  // This deletes the mode widgets (ie management/installation widgets)

  void setupStatusBar();
  // This sets up the status bar

  void arrangeWidgets();
  // This arranges the widgets in the window (should be called after a
  // resize event)

  KFileDialog* getFileDialog(const char* captiontext);


  ///////////// SLOTS --------------------------------------------------------
public slots:
  void dropAction(KDNDDropZone *);
  // This is called when a URL has been dropped in the drop zone

  void modeFinished(int mode, pkgInterface *interface, int refresh);
  // This is called when the mode `mode' has finished.  KPACKAGE should
  // then change modes appropriately

  void fileOpen();
  // This is called when File->Open is selected from the menu

  void clearSelected();
  // clear package selections

  void expandTree();
  void collapseTree();
  // expand and collapse file tree

  void fileOpenUrl();
  // menu item FIle->OpenUrl

  void toggleSelect();
  // toggle multiselct mode on package tree
  void find();
  // search for package

  void findf();
  // search for file in package

  void fileQuit();
  // This is called when File->Quit is selected from the menu

  void reload();
  // reload file package infomation

  void helpAbout();
  // This is called when Help->About is selected from the menu

  void helpHelp();
  // This is called when Help->Help is selected from the menu

  ///////////// SIGNALS ------------------------------------------------------

  ///////////// DATA ---------------------------------------------------------
public:

  enum { Management, Installation } ; 
  // Widget modes

  KConfig *config;
  // pointer to kconfig object

  managementWidget *management;
  // management widget

  QString save_url;
  // save the URL entered

private:
  int mode;
  // Widget mode

  // Menu item identifiers

  QFrame *statusbar;	    
  // the status bar

  KProgress *processProgress;
  // Progress bar for showing progress

  QLabel *status;
  // The actual status

  KDNDDropZone *dropzone;
  // The dropzone where URLs can be dropped

  KFileDialog *file_dialog;
  /// If we load a file from the net this is the corresponding URL

  Search *srchdialog;
  // find package dialog

  FindF *findialog;
  // find file dialog
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class KPKG : public KTopLevelWidget
{
  Q_OBJECT;

public:

  KPKG(KConfig *_config,  const char *name=0);
  // Constructor

  ~KPKG();
  // Destructor

  void add_recent_file(const char* newfile);
  // keep list of files accessed

  void readSettings();
  void writeSettings();
  // write and write settings

  void saveProperties(KConfig *config);
  void readProperties(KConfig *config);
  // save and read restart sstate

  void disableMenu();
  void enableMenu();	
  // enable/deisable menu elements

  void setSelectMode(bool mult);
  // set selection mode in menu item

  KMenuBar *menubar;
  KToolBar *toolbar;
  //  menu and tool bars

  QPopupMenu *recentpopup;

#if QT_VERSION >= 200
  KAccelMenu *filemenu, *helpmenu, *packmenu, *options, *caches, *locationpopup;
#else
  QPopupMenu *filemenu, *helpmenu, *packmenu, *options, *caches, *locationpopup;
#endif
  // The menu bar and its popups

  KPACKAGE *kp;
  // The part under the menu bar

  bool prop_restart;
  // indicates a restart from saved state

  QStrList urlList;
  // For multiple drops save list of URL's for processing

  Options *optiondialog;
  // Options dialog

  KConfig *config ;
  // Saved config information

  KAccel *keys;

private:
  void setupMenu();
  // This sets up the menubar

  void setupToolBar();
  // Setup tool bar

  QStrList recent_files;

  int toolbar1, toolID, selectID;
  // refrences to  toolbar  and menu items

  bool hide_toolbar;
  // don't display toolbar

  int fobut, fibut, robut;
  // toolbar buttons

public slots:

  void openRecent(int);
  // open file from list of recently opened ones

  void setOptions();
  // set options
  
  void setKeys();
  
  void saveSettings();
  // save config
  
  void toggleToolBar();
  // toggle existance of tool bar

  void clearPCache();
  // Clear package cache

  void clearDCache();
  // Clear directory cache

protected:
    void closeEvent ( QCloseEvent *e);
};

//////////////////////////////////////////////////////////////////////////////

extern KPKG *kpkg;
extern KApplication *app;

extern pkgInterface *kpinterface[];
extern const int kpinterfaceN;

extern void KpMsg(const char *lab, const char *format, const char *str, bool stop);
extern void KpMsgE(const char *format, const char *str, bool stop);
#endif



