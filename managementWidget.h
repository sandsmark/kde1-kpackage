////////////////////////////////////////////////////////////////////////
// $Id: managementWidget.h 20671 1999-05-02 14:28:37Z coolo $	
// File  : managementWidget.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This widget is used to provide the management mode of ksetup.
// There are two subwidgets; firstly a tree list showing all the
// currently installed packages and secondly a display of the currently
// selected package's properties.
//
// There are also some control buttons which allow the currently
// selected package to be uninstalled or verified.
///////////////////////////////////////////////////////////////////////

#ifndef MANAGEMENTWIDGET_H
#define MANAGEMENTWIDGET_H

#include "config.h"
// Standard Headers

// Qt Headers
#include <qframe.h>
#include <qpushbt.h>
#include <qlist.h>
#include <qstring.h>
#include <qlayout.h>
#include <qlistview.h>

// KDE headers

// ksetup headers
#include "packageInfo.h"
#include "ktvitem.h"

class packageDisplayWidget;
class packageInfo;
class KNewPanner;


class managementWidget : public QFrame
{
  Q_OBJECT;

  ///////////// METHODS ------------------------------------------------------
public:
  managementWidget(QWidget *parent=0, const char *name=0);
  // Constructor

  ~managementWidget();
  // Destructor

  QListViewItem *search(const char *str, const char* head,
			QListViewItem  *start = 0);
  QListViewItem *search(const char *str, bool subStr, bool wrap, bool start);
  // search for a package in tree

  QListViewItem *updatePackage(packageInfo *pki, bool install);
  // update package in treelist

  void setPSeparator(int p);
  int getPSeparator();
  // return position of panel seperator

  void expandTree(QListView *list);
  // expand package tree

  void expandTree(QListViewItem *item);
  // expand package sub-tree

  void collapseTree(QListView *list);
  // semi-collapse package tree

  void collapseTree(QListViewItem *item);
  // semi-collapse package sub-tree

  void toggleSelect();
  // toggles muliselect on package tree

  void writeTreePos();
  void readTreePos();
  // save and restore column positions

  void findSelected(QListViewItem *item);
  // generate list of selected tree items
  void clearSelected(QListViewItem *item);
  // unselect selected tree items

protected:
  void resizeEvent(QResizeEvent *re);
  // This is called when the widget is resized

private:
  void setupWidgets();
  // This sets up the sub-widgets

  void setupInstButton(packageInfo *p);
  // Set button for inst or uninst

  void setStatus();
  // Set status info on bottom line

  void arrangeWidgets();
  // This arranges the widgets in the window (should be called after a
  // resize event)

  ///////////// SLOTS   ------------------------------------------------------
 public slots:
 void collectData(bool refresh);
  // This collects data about all the packages installed.
  // The list tree is filled with this data.  Whenever something happens
  // that requires data to be (re)collected a signal connected to this slot
  // should be emitted.  This function can also be called directly.

  void rebuildListTree();
  // This rebuilds the list tree.  This would normally be called if the
  // data contained about the packages has been changed (e.g. a verification
  // failed / succeeded).

 private slots:
 void packageHighlighted(QListViewItem *);
  // This is called when a package has been highlighted in the list tree

  void uninstallClicked();
  // This is called when the uninstall button has been clicked

  void examineClicked();
  // This is called when the examine button has been clicked

  void uninstallMultClicked();
  // This is called when the uninstall button has been clicked in multi mode

  void installMultClicked();
  // This is called when the install button has been clicked in multi mode

void currentChanged(QListViewItem *p);
 ///////////// SIGNALS ------------------------------------------------------

  signals:
  void changePackage(packageInfo *p);

  ///////////// DATA ---------------------------------------------------------
private:
  packageInfo *package;

  QPushButton *instButton,*uinstButton;
  // This button is used to (un)install the selected package

  packageDisplayWidget *packageDisplay;
  // This widget displays the package info / file-list

  QBoxLayout *top, *rightbox, *buttons;
  // These are the geometry managers

  QFrame *rightpanel;
  // frame to put QBox in

  KNewPanner *vPan;
  // veritcal panner between panels

  bool searchChild(QListViewItem *it);
  // recurse thru the display tree looking for 'str'

  QListViewItem *searchCitem;
  bool searchSkip, searchSubstr;
  const char *searchStr;
  QListViewItem  *searchResult;
  // globals used by searchChild for start from current position,
  // skip to current item before search flag, substring search flag, 
  // search string, result item (if found)

  // flag skipping in searchChild

  QList<KTVItem> selList;
  // list for selected packages

public:
  QList<packageInfo> *installedPackages;
  // The list of packages

  QDict<packageInfo> *dirInstPackages;
  // maps installed package name to package

  QDict<packageInfo> *dirUninstPackages;
  // maps uninstalled package name to package

  QDict<packageInfo> *dirInfoPackages;
  // maps Info package name to package

  QListView *treeList;
  // This is the tree list where all the packages / groups are displayed
};

#endif



