/////////////////////////////////////////////////////////////////////////
// $Id: packageDisplay.h 20671 1999-05-02 14:28:37Z coolo $	
// File  : packageDisplay.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This widget is used to display information and the file list of a
// package.
//
// Package information will be displayed using (another) sub-widget
// that is inherited from a QTableView.
//
// The file list will be displayed using a tree list.
//
// The widget is mainly a QTabDialog with two tabs: Info and FileList.
// The Info tab is the default one.
//
// It may prove that the tab dialog idea doesn't work. Something else
// will have to be done in that case!!!
/////////////////////////////////////////////////////////////////////////

#ifndef PACKAGEDISPLAY_H
#define PACKAGEDISPLAY_H
#include "config.h"

// Qt Headers
#include <qframe.h>
#include <qtabbar.h>

// k headers

class packagePropertiesWidget;
class packageInfo;
class QListView;
class QListViewItem;

class packageDisplayWidget : public QFrame
{
  Q_OBJECT;

  ///////////// METHODS ------------------------------------------------------
public:
  packageDisplayWidget(QWidget *parent=0, const char *name=0);
  // Constructor

  ~packageDisplayWidget();
  // Destructor

  void noPackage();
  // clear package display in right panel

protected:
  void resizeEvent(QResizeEvent *re);
  // This is called when the widget is resized

private:
  void setupWidgets();
  // This sets up the sub-widgets

  void arrangeWidgets();
  // This arranges the widget in the window (should be called after a
  // resize events

  void updateFileList();
  // This updates the file list to match that found with the currently
  // selected package
  
  ///////////// SLOTS --------------------------------------------------------
public slots:
  void changePackage(packageInfo *p);

  void tabSelected(int tab);

  void openBinding(QListViewItem *);

  ///////////// SIGNALS ------------------------------------------------------

  ///////////// DATA ---------------------------------------------------------
private:
  packageInfo *package;
  // the currently selected package

  QTabBar *tabbar;
  // The tab bar

  QListView *fileList;
  // This holds the file list (and is used as a page on the tab dialog)

  QPixmap *tick, *cross, *question, *blank;
  // The pixmaps for the filelist


  packagePropertiesWidget *packageProperties;
  // This displays the package properties (and is used as a page on the
  // tab dialog)

  bool initList;
  // True is file list has been initialised
};
#endif
