//////////////////////////////////////////////////////////////
//      $Id: pkgInterface.h 19122 1999-04-04 15:25:20Z toivo $ 
//
// Author: Toivo Pedaste
//

#ifndef PKG_IFACE_H
#define PKG_IFACE_H

#include "../config.h"
#include <qlist.h>
#include <qstring.h>

#include <kiconloader.h> 
#include <kdir.h>

#include "packageInfo.h"
#include "procbuf.h"

class packageInfo;
class installationWidget;
class  pkginstallDialogMult;
class pkguninstallDialog;
class pkguninstallDialogMult;
class Locations;
class LcacheObj;
class cacheObj;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
typedef struct param {
  const char *name;
  bool init;
  bool invert;
} param;
// Structure for flags to install and uninstall
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class pkgInterface: public QObject 
{
  Q_OBJECT
 
public:
  pkgInterface();
  virtual ~pkgInterface();
  
  virtual bool isType(char *buf, const char *fname) =  0;
  // looks at start of file to check that package is correct type

  virtual param *initinstallOptions() = 0;
  virtual param *inituninstallOptions() =  0;
  // return appropriate param blocks

  virtual packageInfo *getPackageInfo(char mode, const char *name, const char *version) =  0;
  // get info on installed or uninstalled package. version is only set if
  // mode is 'i' (ie, if the package is already installed).

  virtual QList<char> *getFileList(packageInfo *p) =  0;
  // get list of files in the package

  virtual QList<char> *depends(const char *name, int src);
  // check dependencies for package

  virtual QList<char> *verify(packageInfo *p, QList<char> *files);
  // check the installed files in a package

  virtual int uninstall(int uninstallFlags, QList<packageInfo> *p);
  virtual int uninstall(int uninstallFlags, packageInfo *p);
  // uninstall package or packages

  virtual int install(int installFlags, QList<packageInfo> *p);
  virtual int install(int installFlags, packageInfo *p);
  // install package or packages

  virtual int doUninstall(int installFlags, QString packs);
  virtual int doInstall(int installFlags, QString packs);

  virtual QString FindFile(const char *name) = 0;
  // search for packages containg a file

  virtual bool parseName(QString name, QString *n, QString *v) = 0;
  // breakup file name into package name and version

  virtual void listPackages(QList<packageInfo> *pki);
  // scan various locations for list of packages

  virtual void listInstalledPackages(QList<packageInfo> *pki) = 0;
  // produce list of currently installed packages

  virtual void smerge(packageInfo *p);
  // merge in package info entry

  QString getDir(cacheObj *cp);
  // list directory local or remote

  void listDir(QList<packageInfo> *pki, QString fname, QString dir);
  // list the packages in a directory

  void filesDir(KFileInfoList *lst);
  // process the directory entries
  
  packageInfo *collectDir(QString name, QString size, QString dir);
  // build packageInfo object from directory entry

  QString getPackList(cacheObj *cp);
  // get packages information file

  ///////////// DATA ///////////////////////
  installationWidget *installation;
  pkginstallDialogMult *installationMult;
  pkguninstallDialog *uninstallation;
  pkguninstallDialogMult *uninstallationMult;
  // install and uninstall widgets for this package type

  QString icon;
  // name icon file
  QString head;
  // capitalized name of package type
  QPixmap *pict, *bad_pict, *new_pict, *updated_pict;
  // icons for package states
  QPixmap *folder;
  // icon for package group

  Locations *locatedialog;
  // dialog for setting the locations of  uninstalled packages
  LcacheObj *packageLoc;
  // List of locations of uninstalled pacckages
  
  FILE *tmpFile;
  QString tmpDir;
  KDir *kdir;
  bool dirOK;
  // variables related to reading packages from directories

  char *packagePattern;
  QString queryMsg;
  char  *typeID;
  // Parameters for reading packages from directories

  procbuf reader;
  const char *procMsg;
  // for running processes

public slots:
  virtual void setLocation() = 0;
  virtual void setAvail(LcacheObj *) = 0;
  void fileDir(KFileInfo *);
  void finishedDir();
  void errorDir(int, const char *);
};

extern KIconLoader  *globalKIL;
#endif





