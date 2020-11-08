//////////////////////////////////////////////////////////////
// 	$Id: fbsdInterface.h 19122 1999-04-04 15:25:20Z toivo $
//
// Author: Alex Hayward
//
//////////////////////////////////////////////////////////////

#ifndef FBSD_IFACE_H
#define FBSD_IFACE_H

#include "../config.h"

//#ifdef HAVE_FBSD_PKGTOOLS

#include <qlist.h>

#include <kdir.h>

#include "packageInfo.h"
#include "pkgInterface.h"

class KDir;
class cacheObj;

class fbsdInterface : public pkgInterface
{
 Q_OBJECT

public:
  fbsdInterface();
  ~fbsdInterface();
  
  bool isType(char *buf, const char *fname);
  param *initinstallOptions();
  param *inituninstallOptions();
  packageInfo *getPackageInfo(char mode, const char *name, const char *version);
  QList<char> *getFileList(packageInfo *p);
//  QList<char> *verify(packageInfo *p, QList<char> *files);

  int uninstall(int uninstallFlags, QList<packageInfo> *p);
  int uninstall(int uninstallFlags, packageInfo *p);
  int doUninstall(int uninstallFlags, QString packs);

//  int install(int installFlags, QList<packageInfo> *p);
//  int install(int installFlags, packageInfo *p);
  int doInstall(int installFlags, QString packs);

  QString FindFile(const char *name);
  void collectDepends(packageInfo *p, const char *name, int src);
  bool parseName(QString name, QString *n, QString *v);

  void listInstalledPackages(QList<packageInfo> *pki);
  void listPackages(QList<packageInfo> *pki);

public slots:
  void setLocation();
  void setAvail(LcacheObj *);

private:
  /**
   * @short Add the name and version identifiers to a QDict<QString>.
   *
   * name is parsed in to name and version and these are added to 
   * d. Errors are handled.
   */
  void addNV(QDict<QString> *d, const char *name);
};

/**
 * @short Ports description linked list item
 *
 * Each item in the list describes one port from the ports collection.
 */
class bsdPortsIndexItem {
public:
  /**
   * desc is a line from the INDEX file (/usr/ports/INDEX under FreeBSD)
   * which has a particular format:
   *
   * name|port path|inst prefix|1 line commect|DESCR file|maintainer|categories|build-deps|run-deps
   *
   * Multiple space separated categories may be specified.
   *
   * desc must remain allocated (ie, its not copied) will be modified.
   *
   * binaries should be true if this is a binary package.
   *
   * dname is the name of the base directory of this ports/packages tree.
   */
  bsdPortsIndexItem(char *desc, bool binaries, QString dname);

  /** @short true if this has a binary packages. */
  bool bin;

  /** @short true if this has a source port available. */
  bool port;

  /** @short true if this package is installed (set in listInstalledPackages) */
  bool installed;

  /** @short The next item in this linked list */
  bsdPortsIndexItem *next;

  const char *name;
  const char *path;
  const char *prefix;
  const char *comment;
  const char *desc_path;
  const char *maint;
  const char *cats;
  const char *bdeps;
  const char *rdeps;
  QString bin_filename;
  QString bin_filename_base;
  QString port_dirname;

  /**
   * @short Find a port from the ports index (by name).
   */
  static bsdPortsIndexItem *find(const char *name);

  /**
   * @short A hash table based on the value returned by calc_hash1 or hash1;
   */
  static bsdPortsIndexItem *lists[256];

  /**
   * @short Given the path to an INDEX file process each port in it.
   *
   * binaries should be true if the file is an index for packages, false for ports.
   * dname is the base directory.
   */
  static void processFile(const char *fname, bool binaries, const char *dname);

private:
  unsigned int name_hash;
  static unsigned char calc_hash1(const char *name);
  static unsigned int calc_hash4(const char *name);
  static unsigned char hash1(unsigned int hash4);
};

#endif



