//////////////////////////////////////////////////////////////
//      $Id: kissInterface.h 20708 1999-05-03 07:21:48Z coolo $ 
//
// Author: Toivo Pedaste
//
//////////////////////////////////////////////////////////////

#ifndef KISS_IFACE_H
#define KISS_IFACE_H

#include "../config.h"

#include <qlist.h>
#include <kprocess.h>
#include <kfm.h>

#include "procbuf.h"
#include "pkgInterface.h"

class packageInfo;
class updateLoc;
class cacheObj;

class KISS: public pkgInterface
{
  Q_OBJECT

public:
  KISS();
  ~KISS();
  
  bool isType(char *buf, const char *fname);
  param *initinstallOptions();
  param *inituninstallOptions();
  packageInfo *getPackageInfo(char mode, const char *name, const char *version);
  QList<char> *getFileList(packageInfo *p);

  QString FindFile(const char *name);
  bool parseName(QString name, QString *n, QString *v);

public slots:
  void setLocation();
  void setAvail(LcacheObj *);

private:
  packageInfo* collectInfo(const char *inp);
   void listInstalledPackages(QList<packageInfo> *pki);
 
  int doUninstall(int installFlags, QString packs);
  int doInstall(int installFlags, QString packs);

};

#endif



