//////////////////////////////////////////////////////////////         
//      $Id: cache.cpp 19960 1999-04-17 22:36:57Z coolo $ 
//
// Author: Toivo Pedaste
//
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "kpackage.h"
#include "options.h"
#include "cache.h"
#include <klocale.h>

extern Params *params;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
cacheObj::cacheObj(QString Pbase, QString Plocation, QString PcacheFile)
{
  base = Pbase;
  location = Plocation;
  cacheFile = PcacheFile;
}

cacheObj::~cacheObj()
{}

QString cacheObj::PDir()
{
  QString tmpd = QDir::homeDirPath();
  struct stat buf;
  stat(tmpd.data(),&buf);
    
  tmpd += "/.kpackage/";

  QDir d(tmpd.data());
  if (!d.exists()) {
    if (!d.mkdir(tmpd.data())) {
      KpMsgE(i18n("Cannot create directory %s"),
	     tmpd.data(),TRUE);
      tmpd = "";
    } else {
      chown(tmpd.data(),buf.st_uid,buf.st_gid);
    }
  }
  return tmpd;
}

QString cacheObj::CDir()
{
  QString tmpd = PDir();
  if (!tmpd.isEmpty()) {
    struct stat buf;
    stat(tmpd.data(),&buf);
    
    tmpd += "dir/";

    QDir d(tmpd.data());
    if (!d.exists()) {
      if (!d.mkdir(tmpd.data())) {
	KpMsgE(i18n("Cannot create directory %s"),
	       tmpd.data(),TRUE);
	tmpd = "";
      } else {
	chown(tmpd.data(),buf.st_uid,buf.st_gid);
      }
    }
  }
  return tmpd;
}

int cacheObj::newDCache(const char *url, QString fn, QString *fname) {
  KURL *u = new KURL(url );
  if ( u->isMalformed() ) {
    KpMsgE(i18n("Malformed URL: %s"),url,TRUE);
    return -1;
  }
  //  printf("newcache %s %s\n",url,fn.data());

  QString tmpd = cacheObj::CDir();
  if (tmpd.isEmpty()) {
    return -1;
  } else {
    if (strcmp(u->protocol(), "file") == 0) {
      *fname = u->path();
      return 0;
    }

    *fname = tmpd + fn;

    if (params->DCache == Params::NEVER) {
      return 1;
    }

    QFileInfo f(fname->data());

    if (f.exists() && f.size() > 0) {
      return 0;
    } else {
      if (f.size() == 0)
	rmDCache(*fname);
      return 1;
    }
  }
}

void  cacheObj::rmDCache(QString fn) {
  QString tmpd = cacheObj::CDir();
  tmpd += fn;

  if (!tmpd.isEmpty()) {
    //    printf("rm=%s\n",tmpd.data());
    unlink(tmpd.data());
  }
}

void  cacheObj::clearDCache() {
  QString tmpd = cacheObj::CDir();

  if (!tmpd.isEmpty()) {
    QDir d(tmpd.data());
    if (const QFileInfoList *list = d.entryInfoList()) {
      QFileInfoListIterator it( *list ); 
      QFileInfo *fi;                          

      while ( (fi=it.current()) ) {
	if (!fi->isDir()) {
	  QString s = tmpd.data();
	  s += fi->fileName();
	  //	  printf("RM=%s\n",s.data());
	  unlink(s.data());
	}
	++it;      
      }
    }
  }
}

void  cacheObj::clearPCache() {
  QString tmpd = cacheObj::PDir();

  if (!tmpd.isEmpty()) {
    QDir d(tmpd.data());
    if (const QFileInfoList *list = d.entryInfoList()) {
      QFileInfoListIterator it( *list ); 
      QFileInfo *fi;                          

      while ( (fi=it.current()) ) {
	if (!fi->isDir()) {
	  QString s = tmpd.data();
	  s += fi->fileName();
	  //	  printf("RM=%s\n",s.data());
	  unlink(s.data());
	}
	++it;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
LcacheObj::LcacheObj()
{
  setAutoDelete(TRUE);
}

LcacheObj::~LcacheObj()
{
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

