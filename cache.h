//////////////////////////////////////////////////////////////
//      $Id: cache.h 16726 1999-02-11 04:18:43Z toivo $ 
//
// Author: Toivo Pedaste
//

#ifndef CACHE_H
#define CACHE_H

#include <qdir.h>
#include <qlist.h>

#include <kurl.h>

#include "../config.h"
#include "packageInfo.h"

class Locations;
class LcacheObj;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class cacheObj
{
public:
  QString base;
  QString location;
  QString cacheFile;

  cacheObj(QString Pbase, QString Plocation, QString PcacheFile);
  ~cacheObj();

  static QString PDir();
  static QString CDir();
  static  int newDCache(const char *url, QString fn, QString *fname);
  static  void rmDCache(QString fn);

  static void clearDCache();
  static void clearPCache();
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class LcacheObj: public QList<cacheObj>
{
public:
  LcacheObj();
  ~LcacheObj();
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#endif
