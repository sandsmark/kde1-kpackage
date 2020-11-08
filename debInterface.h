//////////////////////////////////////////////////////////////
//      $Id: debInterface.h 20708 1999-05-03 07:21:48Z coolo $ 
//
// Author: Toivo Pedaste
//
//////////////////////////////////////////////////////////////

#ifndef DEB_IFACE_H
#define DEB_IFACE_H

#include "config.h"

#include <qlist.h>
#include <kprocess.h>
#include <kfm.h>

#include "procbuf.h"
#include "pkgInterface.h"

class packageInfo;
class updateLoc;
class cacheObj;

class DEB: public pkgInterface
{
  Q_OBJECT

public:
  DEB();
  ~DEB();
  
  bool isType(char *buf, const char *fname);
  void listPackages(QList<packageInfo> *pki);
  param *initinstallOptions();
  param *inituninstallOptions();
  packageInfo *getPackageInfo(char mode, const char *name, const char *version);
  QList<char> *getFileList(packageInfo *p);

  void listInstalledPackages(QList<packageInfo> *pki);

  QString FindFile(const char *name);
  bool parseName(QString name, QString *n, QString *v);

public slots:
  void setLocation();
  void setAvail(LcacheObj *);

private:
  packageInfo* collectInfo(const char *inp);
  void listPackList(QList<packageInfo> *pki, const char *fname, cacheObj *cp);
  int dpkgChange(int del);

  int doUninstall(int installFlags, QString packs);
  int doInstall(int installFlags, QString packs);

};

#endif



