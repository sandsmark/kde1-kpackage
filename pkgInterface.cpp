//////////////////////////////////////////////////////////////         
//      $Id: pkgInterface.cpp 20307 1999-04-25 15:38:26Z toivo $ 
//
// Author: Toivo Pedaste
//
#include <klocale.h>
 
#include "kpackage.h"
#include "pkgInterface.h"
#include "options.h"
#include "cache.h"
#include "updateLoc.h"
#if QT_VERSION >= 200
#include "kio.h"
#endif

extern Params *params;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
pkgInterface::pkgInterface() : QObject() 
{
  packageLoc = 0;
  bad_pict = 0;
  new_pict = 0;
  updated_pict = 0;
  kdir = NULL;
  tmpFile = 0;
  packageLoc = 0;

  folder = new QPixmap();
  *folder = globalKIL->loadIcon("mini/folder.xpm");
}

//////////////////////////////////////////////////////////////////////////////
pkgInterface::~pkgInterface()
{
  if (locatedialog)
    delete locatedialog;
  if (packageLoc)
    delete packageLoc;
  delete pict;
  if (bad_pict)
    delete bad_pict;
  if (updated_pict)
    delete updated_pict;
  if (new_pict)
    delete new_pict;
  delete folder;
}

//////////////////////////////////////////////////////////////////////////////
QList<char> *pkgInterface::depends(const char *name, int src) {return 0;}

int pkgInterface::doUninstall(int installFlags, QString packs) {return 0;}
int pkgInterface::doInstall(int installFlags, QString packs) {return 0;}

////////////////////////////////////////////////////////////////////////////

void pkgInterface::listPackages(QList<packageInfo> *pki)
{ 
  QString s;
  cacheObj *cp;

  listInstalledPackages(pki);
  if (params->DisplayP != Params::INSTALLED) {
    if (packageLoc) {
      for (cp = packageLoc->first(); cp != 0; cp = packageLoc->next()) {
	s = getDir(cp);
	if (s != "") {
	  listDir(pki,s,cp->location);
	}
      }
    }
  }
}

void pkgInterface::smerge(packageInfo *p)
{;}

void pkgInterface::listDir(QList<packageInfo> *pki, QString fname, QString dir) 
{
  QString name, size, rfile;
  packageInfo *p;

  QString sline = queryMsg.data();
  sline += fname;
  kpkg->kp->setStatus(sline.data());

  QDir d(fname.data(),packagePattern);
  if (d.exists()) {
    QString pn;
    const QFileInfoList *list = d.entryInfoList();
    QFileInfoListIterator it( *list );      // create list iterator
    QFileInfo *fi;                          // pointer for traversing

    while ( (fi=it.current()) ) {           // for each file...
      if (params->PkgRead) {
	rfile = dir + "/";
	rfile += fi->fileName();
	p = getPackageInfo('u',rfile.data(), 0);
	if (p) {
	  p->info->insert("filename", new QString(fi->fileName()));
	  p->info->insert("base", new QString(dir));
 	}
      } else {
	p = collectDir(fi->fileName(),pn.setNum(fi->size()).data(),dir);
      }
      if (p) {
	smerge(p);
	if (!p->update(pki, typeID, FALSE))
	  delete p;
      }
      ++it;                               // goto next list element
    }
  } else {
    FILE *f = fopen(fname.data(),"r");
    char buf[201];
    if (f) {
      while (fgets(buf,200,f)) {
	name = buf;
	name.truncate(name.length() - 1);
	if (fgets(buf,200,f)) {
	  size = buf;
	  size.truncate(size.length() - 1);
	} else
	  size = "";
	packageInfo *p = collectDir(name,size,dir);
	if (p) {
	  smerge(p);
	  if (!p->update(pki, typeID, FALSE))
	    delete p;
	}
      }
      fclose(f);
    }
  }
}

packageInfo *pkgInterface::collectDir(QString name, QString size, QString dir) 
{
  QString n,v;

  if (parseName(name, &n, &v)) {
    QDict<QString> *a = new QDict<QString>;
    a->setAutoDelete(TRUE);

    a->insert("group", new QString("NEW"));
    a->insert("name", new QString(n));
    a->insert("version", new QString(v));
    a->insert("file-size", new QString(size));
    a->insert("filename", new QString(name));
    a->insert("base", new QString(dir));

    packageInfo *i = new packageInfo(a,this);
    i->packageState = packageInfo::AVAILABLE;
    //    i->packageState = packageInfo::NEW;
    return i;
    } 
  return 0;
}

QString pkgInterface::getPackList(cacheObj *cp) 
{
  QString tmpf;
  int res;

  QString url = cp->location;

  if ((res = cacheObj::newDCache(url, cp->cacheFile, &tmpf))) {
    if (res < 0)
      return "";

    unlink(tmpf.data());
    if (kpkg)
      kpkg->kp->setStatus(i18n("Calling KFM"));

#if QT_VERSION < 200
    if (KFM::download(url, tmpf)) {
#else
    Kio kio;
    if (kio.download(url, tmpf)) {
#endif      
      if (kpkg)
	kpkg->kp->setStatus(i18n("KFM finished"));
      QFileInfo f(tmpf.data());
      if (!(f.exists() && f.size() > 0)) {
	unlink(tmpf.data());
	return "";
      } else {
	return tmpf;
      }
    } else {
      if (kpkg)
	kpkg->kp->setStatus(i18n("KFM failed"));
      return "";
    }
  } else {
    return tmpf;
  }
}

QString pkgInterface::getDir(cacheObj *cp) {
  QString string = "";
  int res;

  QString url = cp->location;

  if ((res = cacheObj::newDCache(url, cp->cacheFile, &tmpDir))) {
    if (res < 0)
      return string;

#if QT_VERSION < 200
        if (!kdir) {
          kdir = new KDir();
          connect(kdir,SIGNAL(dirEntry(KFileInfo *)),
    	      SLOT(fileDir(KFileInfo *)));
          connect(kdir,SIGNAL(finished()),SLOT(finishedDir()));
          connect(kdir,SIGNAL(error(int, const char *)),
    	      SLOT(errorDir(int, const char *)));
        }

        unlink(tmpDir.data());
        tmpFile = fopen(tmpDir.data(),"w");
        if (!tmpFile) {
          KpMsgE(i18n("Cann't write to file %s\n"),
    	     tmpDir.data(),FALSE);
          return string;
        }
        kdir->setURL(url);

        dirOK = TRUE;
        const KFileInfoList *lst = kdir->entryInfoList();
        filesDir((KFileInfoList *)lst);

        app->enter_loop(); 
        if (dirOK) {
#else
    Kiod kiod;
    if (kiod.listDir(url,tmpDir)) {
#endif      
      return tmpDir;
    } else {
#if QT_VERSION < 200
      delete kdir;
      kdir = 0;
#endif
      KpMsgE(i18n("Cann't read directory %s"),url.data(),FALSE);
      unlink(tmpDir.data());
      return string;
    }
  } else {
    return tmpDir;
  }
}

void pkgInterface::finishedDir()
{
  if (tmpFile) {
    //    printf("exit_loop\n");
    app->exit_loop();
    fclose(tmpFile);
    dirOK = TRUE;
  }
}

void pkgInterface::errorDir(int n, const char *err)
{
  if (tmpFile) {
    printf("E=%d %s\n",n,err);
    //    printf("exit_loop\n");
    app->exit_loop();
    fclose(tmpFile);
    dirOK = FALSE;
  }
}

void pkgInterface::filesDir(KFileInfoList *lst)
{
  KFileInfo *inf;

  for (inf = lst->first(); inf != 0; inf = lst->next()) {
    fileDir(inf);
  }	
}

void pkgInterface::fileDir(KFileInfo *inf)
{
  if (KDir::match(packagePattern,inf->fileName())) {
    fprintf(tmpFile,"%s\n%d\n",inf->fileName(),inf->size());
  }
}

//////////////////////////////////////////////////////////////////////////////
QList<char> *pkgInterface::verify(packageInfo *package, QList<char> *files)
{
  int  p = 0;
  uint c = 0;
  QList<char> *errorlist = new QList<char>;
  QDir d;

  QListIterator<char> it(*files);
  uint step = (it.count() / 100) + 1;

  kpkg->kp->setStatus(i18n("Verifying"));
  kpkg->kp->setPercent(0);

  char *s;
  for(; (s = it.current()); ++it) {

    // Update the status progress
    c++;
    if(c > step) {
      c=0; p++;
      kpkg->kp->setPercent(p);
    }

    if (!d.exists(s)) {
      errorlist->append(strdup(s));
    }	
  }

  kpkg->kp->setPercent(100);
  return errorlist;
}

//////////////////////////////////////////////////////////////////////////////
int pkgInterface::uninstall(int uninstallFlags, packageInfo *p)
{
  QString packs;
  packs = p->getProperty("name")->data();

  return doUninstall(uninstallFlags, packs);
}

//////////////////////////////////////////////////////////////////////////////
int pkgInterface::uninstall(int uninstallFlags, QList<packageInfo> *p)
{
  QString packs = "";
  packageInfo *i;

  for (i = p->first(); i!= 0; i = p->next())  {
    packs += *i->getProperty("name");
    packs += " ";
  }
  return doUninstall( uninstallFlags, packs);
}
//////////////////////////////////////////////////////////////////////////////

int pkgInterface::install(int installFlags, packageInfo *p)
{
  QString fname = p->fetchFilename();

  return doInstall(installFlags, fname);
}

//////////////////////////////////////////////////////////////////////////////
int pkgInterface::install(int installFlags, QList<packageInfo> *p)
{
  QString packs = "";
  packageInfo *i;

  for (i = p->first(); i!= 0; i = p->next())  {
    QString fname = i->fetchFilename();
    if (fname != "") {
      packs += fname;
      packs += " ";
    }
  }
  return doInstall(installFlags, packs);
}

