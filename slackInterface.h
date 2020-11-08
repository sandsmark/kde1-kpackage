//////////////////////////////////////////////////////////////
//      $Id: slackInterface.h 20708 1999-05-03 07:21:48Z coolo $ 
//
// Author: Toivo Pedaste
//
//////////////////////////////////////////////////////////////

#ifndef SLACK_IFACE_H
#define SLACK_IFACE_H

#include "../config.h"

#include <qlist.h>
#include <kprocess.h>
#include <kfm.h>

#include "procbuf.h"
#include "pkgInterface.h"

class packageInfo;
class updateLoc;
class cacheObj;

class SLACK: public pkgInterface
{
  Q_OBJECT

public:
  SLACK();
  ~SLACK();
  
  bool isType(char *buf, const char *fname);
  param *initinstallOptions();
  param *inituninstallOptions();
  packageInfo *getPackageInfo(char mode, const char *name, const char *version);
  QList<char> *getFileList(packageInfo *p);

  QString FindFile(const char *name);
  bool parseName(QString name, QString *n, QString *v);

  int install(int installFlags, QList<packageInfo> *plist);

public slots:
  void setLocation();
  void setAvail(LcacheObj *);

private:
  packageInfo* collectInfo(const char *inp, int insState);
   void listInstalledPackages(QList<packageInfo> *pki);
 
  int doUninstall(int installFlags, QString packs);
  int doInstall(int installFlags, QString packs);

  void listPackages(QList<packageInfo> *pki);
  void listPackList(QList<packageInfo> *pki, QString s,
		    cacheObj *cp, int insState);

  void initTranslate();

  void smerge(packageInfo *p);

  QDict<char> *trl;
};

#endif



