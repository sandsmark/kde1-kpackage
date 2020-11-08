//////////////////////////////////////////////////////////////
// 	$Id: rpmInterface.h 20996 1999-05-07 14:13:11Z toivo $	
//
// Author: Toivo Pedaste
//
//////////////////////////////////////////////////////////////

#ifndef RPM_IFACE_H
#define RPM_IFACE_H

#include "config.h"

#ifdef HAVE_RPM
extern "C"
{
  #include <rpm/rpmlib.h>
}
#else
  typedef struct  Header{
};
#endif

#include <qlist.h>

#include <kdir.h>

#include "packageInfo.h"
#include "pkgInterface.h"

class KDir;
class cacheObj;

class RPM : public pkgInterface
{
 Q_OBJECT

public:
  RPM();
  ~RPM();
  
  bool isType(char *buf, const char *fname);
  param *initinstallOptions();
  param *inituninstallOptions();
  packageInfo *getPackageInfo(char mode, const char *name, const char *version);
  QList<char> *getFileList(packageInfo *p);
  QList<char> *depends(const char *name, int src);
  QList<char> *verify(packageInfo *p, QList<char> *files);

  int uninstall(int uninstallFlags, QList<packageInfo> *p);
  int uninstall(int uninstallFlags, packageInfo *p);
  int doUninst(int uninstallFlags, char *files[]);

  int install(int installFlags, QList<packageInfo> *p);
  int install(int installFlags, packageInfo *p);
  int doinst(int installFlags, char *files[]);

  QString FindFile(const char *name);
  void collectDepends(packageInfo *p, const char *name, int src);
  bool parseName(QString name, QString *n, QString *v);

public slots:
  void setLocation();
  void setAvail(LcacheObj *);

private:
  packageInfo* collectInfo(Header h);
  QList<char>* collectFileList(Header h);
  void listInstalledPackages(QList<packageInfo> *pki);
  bool rpmSetup;
};

#endif



