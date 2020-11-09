//////////////////////////////////////////////////////////////////////////
// 	$Id: packageInfo.h 20671 1999-05-02 14:28:37Z coolo $	
// File  : packageInfo.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This file contains the definition of the class packageInfo
//
// packageInfo is used to store information regarding an  package.
// This information is normally gained by querying the  database or
// by querying an  package.
//
// The package information consists of a set of properties.  These 
// properties are stored in a dictionary that is passed to the 
// constructor.  The properties can be accessed using the function
// `getProperty'.
//
// In addition, packageInfo objects can place themselves inside
// a tree list with the function `place'.  Doing this creates
// a tree list item object that can be accessed with the function
// `item'.
//////////////////////////////////////////////////////////////////////////

#ifndef PACKAGEINFO_H
#define PACKAGEINFO_H
#include "config.h"

#include <qdict.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qlist.h>
#include <qlistview.h>

class pkgInterface;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class packageInfo
{
public:
  packageInfo(QDict<QString> *_info, pkgInterface *type);
  // Constructor: create a packageInfo object using the property
  // dictionary from _info

  packageInfo(QDict<QString> *_info, QString _filename);
  // Constructor: same as above, but also sets filename to _filename.
  // This is used in the case that the package info was obtained from
  // an uninstalled package.

  ~packageInfo();
  // Distructor

  QString *getProperty(const char *property);
  // returns the property `property' from the dictionary

  void fixup();
  // Initialize fields if missing

  QListViewItem *place(QListView *tree, bool InsertI=FALSE);
  // places the object in the treelist `tree' and initialises
  // `item'.  If necessary, new groups will be added to `tree'.

  QListViewItem *getItem();
  // returns the treelist item object for this package or
  // NULL if the object hasn't been placed

  QDict<QString> *getDict();
  // returns the property dictionary for the package

  void setFilename(const char *f);
  // sets the filename

  QString getUrl();
  // gets the filename

  QString getFilename();
  // gets the filename

  QString fetchFilename();
  // gets the filename, fetching package if necessary

  int newer(packageInfo *p);
  // if package p is newer

  bool update(QList<packageInfo> *pki, const char *exp, bool installed,
	      bool infoPackage = FALSE);
  // insert packgeInfo either installed or not installed

  QDict<QString> *info;
  // This stores the property dictionary of the package

  QListViewItem *item;
  // This stores the tree list item for this package (or NULL if
  // the package hasn't been placed in a tree list)

  pkgInterface *interface;
  // interface points to the class of the package (deb, rpm etc)

  bool smerge(const char *exp);
  // merge with already existing NOLIST package info

  enum {UNSET, AVAILABLE,  INSTALLED, BAD_INSTALL, UPDATED, NEW, NOLIST};
  int packageState;

  bool updated;

private:
  int getDigElement(QString s, int *pos);
  QString getNdigElement(QString s, int *pos);
  // break up version string

  int pnewer(QString s, QString sp);
  // compare parts of a version string
  
  QString url;
  // This stores the filename of the package the info was obtained from.
  // If it is empty then the info was obtained from an installed package.

};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#endif

