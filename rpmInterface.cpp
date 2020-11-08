//////////////////////////////////////////////////////////////         
//      $Id: rpmInterface.cpp 21104 1999-05-09 15:00:41Z toivo $ 
//
// Author: Toivo Pedaste
//
#include "config.h"
#ifdef HAVE_RPM
extern "C"
{
#include <rpm/rpmlib.h>

#ifdef  RPMTAG_EPOCH  
#include <rpm/rpmio.h>
#endif

#include "rpmInstall.h"
#include "rpmVerify.h"

#include <rpm/dbindex.h>
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

#include <kdir.h>
#include <klocale.h>
 
#include "rpmInterface.h"
#include "updateLoc.h"
#include "packageInfo.h"
#include "managementWidget.h"
#include "kpackage.h"
#include "rpmutils.h"
#include "options.h"
#include "cache.h"

#ifndef  RPMTAG_EPOCH  
#define fdOpen open
#define fdClose close
#endif

static param pinstall[] =  {
  {"Upgrade",TRUE,FALSE},
  {"Replace Files",FALSE,FALSE},
  {"Replace Packages",TRUE,FALSE},
  {"Check Dependencies",TRUE,TRUE},
  {"Test (do not install)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

static param puninstall[] =  {
  {"Use Scripts",TRUE,FALSE},
  {"Check Dependencies",TRUE,FALSE},
  {"Test (do not uninstall)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

extern Params *params;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RPM::RPM()
{
  char * rcfile = NULL;
  char *arch = NULL;
#ifndef  RPMTAG_EPOCH   
  char *os = NULL;
  int building = 0;
#endif
   head = "RPM";

  locatedialog = new Locations(i18n("Location of RPM package archives"));
  locatedialog->dLocations(4,6, this, i18n("D"),
  "Rpm","*.rpm", i18n("Location of directories containg RPM packages"));

  connect(locatedialog,SIGNAL(returnVal(LcacheObj *)),
	  this,SLOT(setAvail(LcacheObj *)));
  locatedialog->apply_slot();

  pinstall[0].name = strdup(i18n("Upgrade"));
  pinstall[1].name = strdup(i18n("Replace Files"));
  pinstall[2].name = strdup(i18n("Replace Packages"));
  pinstall[3].name = strdup(i18n("Check Dependencies"));
  pinstall[4].name = strdup(i18n("Test (do not install)"));

  puninstall[0].name = strdup(i18n("Use Scripts"));
  puninstall[1].name = strdup(i18n("Check Dependencies"));
  puninstall[2].name = strdup(i18n("Test (do not uninstall)"));

  icon = "rpm.xpm";
  pict = new QPixmap();
  *pict = globalKIL->loadIcon(icon); 
  bad_pict = new QPixmap();
  *bad_pict = globalKIL->loadIcon("dbad.xpm");
  updated_pict = new QPixmap();
  *updated_pict = globalKIL->loadIcon("rupdated.xpm");
  new_pict = new QPixmap();
  *new_pict = globalKIL->loadIcon("rnew.xpm");

  packagePattern = "*.rpm";
  queryMsg = strdup(i18n("Querying RPM package list: "));
  typeID = "/rpm";

#ifndef  RPMTAG_EPOCH   
  if(rpmReadConfigFiles(rcfile, arch, os, building)) {
    KpMsgE( i18n("Cann't read RPM config files\n"),"",TRUE);
#else
  rpmErrorSetCallback(rpmErr);

  if(rpmReadConfigFiles(rcfile, arch))  
  {
#endif
    rpmSetup = FALSE;
  } else {
    rpmSetup = TRUE;
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
RPM::~RPM()
{
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
param *RPM::initinstallOptions()
{
  return &(pinstall[0]);
}

param *RPM::inituninstallOptions()
{
  return &(puninstall[0]);
}

bool RPM::isType(char *buf, const char *fname)
{
  if ((unsigned char)buf[0] == 0355 && (unsigned char)buf[1] == 0253 &&
      (unsigned char)buf[2] == 0356 && (unsigned char)buf[3] == 0333 ) {
    return true;
  } else
    return false;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


bool RPM::parseName(QString name, QString *n, QString *v)
{
  int d1, d2, s1, s2;

  s2 = name.findRev('.');
  if (s2 > 0) {
    s1 = name.findRev('.',s2-1);
    if (s1 > 0) {
      d2 = name.findRev('-',s1-1);
      if (d2 > 0) {
	d1 = name.findRev('-',d2-1);
	if (d1 < 0)
	  d1 = d2;
	*n = name.left(d1);
	*v = name.mid(d1+1,s1-d1-1);
	return TRUE;
      }
    }
  }
  return FALSE;
}

void RPM::collectDepends(packageInfo *p, const char *name, int src)
{
  QString dlist;
  QList<char> *list = depends(name,src);

  if (list) {
    char *l;
    int i = 0;

    for ( l = list->first(); l != 0; l = list->next(), i++ ) {
      if (i)
	dlist += " , ";
      dlist += l;
    }
    if (dlist.data())
      p->info->insert("unsatisfied dependencies",new QString(dlist));
    if (list)
      delete(list);
  }
}

void RPM::listInstalledPackages(QList<packageInfo> *pki)
{
  int percentage;
  int offset;
  rpmdb db;
  Header h;
  char *prefix = "";
  packageInfo *p;

  if (!rpmSetup)
    return;
    
  kpkg->kp->setStatus(i18n("Querying RPM database for installed packages"));
  percentage=0;
  kpkg->kp->setPercent(percentage);
  
  if ( rpmdbOpen(prefix,&db, O_RDONLY, 0644) )
    return;
  
  offset = rpmdbFirstRecNum(db);
  while (offset)
    {
      percentage+=5;
      if(percentage > 100) percentage=0;
      kpkg->kp->setPercent(percentage);
      h = rpmdbGetRecord(db, offset);
      if (!h) 
	{
	  KpMsgE(i18n("could not read database record\n"),"",TRUE);
	  return;
	}
      
      p = collectInfo(h);
      if (p) {
	if (!p->update(pki, typeID,TRUE))
	  delete p;
      }
      
      headerFree(h);
      offset = rpmdbNextRecNum(db, offset);
    }
  
  kpkg->kp->setPercent(100);
  rpmdbClose(db);
  kpkg->kp->setStatus("");
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// mode: i = query installed    u = query uninstalled
packageInfo *RPM::getPackageInfo(char mode, const char *name, const char *version)
{
  rpmdb db;
#ifdef  RPMTAG_EPOCH
  _FD *fd; 
#else
  int fd;
#endif
  int  rc=0, isSource;
  Header h;
  char *prefix = ""; // this is the root directory -- should be configured!
  packageInfo *pki = 0;;
  
  switch(mode)
    {
      ////////////////////////////////////////////////////////////////////////
      // query an installed package!
    case 'i':
      if (rpmdbOpen(prefix,&db, O_RDONLY, 0644) )
	return 0;

      dbiIndexSet matches;

      rc = findPackageByLabel(db, name, &matches);
      if(rc==1)
	return 0;
	//	KpMsgE(i18n("Package %s is not installed\n"),name,TRUE);
      else if(rc==2)
	KpMsgE(i18n("Error looking for package %s\n"),name,TRUE);
      else
	{
	  int i;
	  for(i=0; i<matches.count; i++)
	    {
	      if(matches.recs[i].recOffset)
		{
		  h = rpmdbGetRecord(db, matches.recs[i].recOffset);
		  if(!h)
		    KpMsgE(  i18n("Could not read database record\n"),"",TRUE);
		  else
		    {
		      pki = RPM::collectInfo(h);
		      headerFree(h);
		    }
		}
	    }

	  collectDepends(pki, pki->getProperty("name")->data(),VERIFY_PACKAGE);

	  dbiFreeIndexRecord(matches);
	}
      rpmdbClose(db);
      break;

      ////////////////////////////////////////////////////////////////////
      // query an uninstalled package      
    case 'u':
      if((fd = fdOpen(name, O_RDONLY, 0)) < 0)
	{
	  KpMsgE(i18n("Problem opening %s\n"),name,TRUE);
	  return pki;
	}
#ifdef  RPMTAG_EPOCH
      if(fd!=0)
#else
      if(fd>=0)
#endif	
	{
	  rc = rpmReadPackageHeader(fd, &h, &isSource, NULL, NULL);
	  fdClose(fd);
	}
      switch(rc)
	{
	case 0:
	  if(!h)
	    KpMsgE(i18n("Old format source packages cannot be queried!\n"),"",TRUE);
	  else
	    {
	      pki = RPM::collectInfo(h);
	      headerFree(h);
	      collectDepends(pki, name, VERIFY_RPM);
	    }
	  break;
	case 1:
	  KpMsgE(i18n("%s does not appear to be a RPM package\n"),name,TRUE);
	  // fallthrough
	case 2:
	  KpMsgE(i18n("Query of %s failed!\n"),name,TRUE);
	  return 0;
	}
      break;
    }
  if (pki)
    pki->updated = TRUE;
  return pki;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define DO(x,z)                                                  \
{                                                                \
  char *k;                                                       \
  headerGetEntry(h, z, &type, (void **) &k, &count);             \
  if(k)                                                          \
    a->insert(x, new QString(k));                                \
}								 \

#define DO2(x,z)                                                 \
{                                                                \
  int_32 k;                                                      \
  char *k2;                                                      \
  headerGetEntry(h, z, RPM_INT32_TYPE, (void **) &k, &count);    \
  if(k)                                                          \
    {                                                            \
      sprintf(k2,"%d",k);                                        \
      a->insert(x, new QString(k2));                             \
      printf(" %s -- %s\n",x,k);                                 \
    }                                                            \
}                                

packageInfo *RPM::collectInfo(Header h)
{
  int_32 type,count;
  time_t dateint;   struct tm * tstruct;   char buf[100];
  
  QDict<QString> *a = new QDict<QString>;
  a->setAutoDelete(TRUE);

  DO("name",RPMTAG_NAME);
  DO("version",RPMTAG_VERSION);
  DO("release",RPMTAG_RELEASE);
  DO("summary",RPMTAG_SUMMARY);
  DO("group",RPMTAG_GROUP);
  DO("distribution",RPMTAG_DISTRIBUTION);
  DO("vendor",RPMTAG_VENDOR);
  DO("packager",RPMTAG_PACKAGER);
  //  DO("source",RPMTAG_SOURCE);

  QString *vers = a->find("version");
  QString *rel = a->find("release");
  if (rel) {
    *vers += "-";
    *vers += *rel;
    a->remove("release");
  }

  char *k;


  // Get nice description
  headerGetEntry(h, RPMTAG_DESCRIPTION, &type, (void **) &k, &count);
  if(k) {
    char *p;
    for (p = k; *p!=0; p++) {	// remove newlines
      if (*p == '\n') {
	if (*(p+1) == '\n') {
	  p++;
	} else if (*(p+1) == ' ') {
	  p++;
	} else
	  *p = ' ';
      }
    }
    a->insert("description",new QString(k));
  }

  // Get nice install time
  headerGetEntry(h, RPMTAG_INSTALLTIME, &type, (void **) &k, &count);
  if(k)
    {
      /* this is important if sizeof(int_32) ! sizeof(time_t) */
      dateint = *(((int_32 *) k) + 0);
      tstruct = localtime(&dateint);      
      strftime(buf, sizeof(buf) - 1, "%c", tstruct);      
      
      a->insert("installtime", new QString(buf));
    }
  
  // Get nice build time
  headerGetEntry(h, RPMTAG_BUILDTIME, &type, (void **) &k, &count);
  if(k)
    {
      /* this is important if sizeof(int_32) ! sizeof(time_t) */
      dateint = *(((int_32 *) k) + 0);
      tstruct = localtime(&dateint);      
      strftime(buf, sizeof(buf) - 1, "%c", tstruct);      
      
      a->insert("build-time", new QString(buf));
    }
  
  // Get nice size
  headerGetEntry(h, RPMTAG_SIZE, &type, (void **) &k, &count);
  if(k)
    {
      sprintf(buf,"%d",*((int_32 *) k) );
      a->insert("size", new QString(buf));
    }
  
  packageInfo *i = new packageInfo(a,this);
  i->packageState = packageInfo::INSTALLED;
  i->fixup();
  return i;
}
#undef DO
#undef DO2

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QList<char> *RPM::getFileList(packageInfo *p)
{

  rpmdb db;
#ifdef  RPMTAG_EPOCH
  _FD *fd;
#else
  int fd;
#endif
  int  rc=0, isSource;
  Header h;
  char *prefix = ""; // this is the root directory -- should be configured!
  char *name;
  char mode;
  
  QString fn = p->getFilename();
  if(!fn.isEmpty())
    mode = 'u';
  else
    mode = 'i';

  QList<char> *filelist=NULL;

  switch(mode)
    {
      ////////////////////////////////////////////////////////////////////////
      // query an installed package!
    case 'i':
      name = p->getProperty("name")->data();
      if (rpmdbOpen(prefix,&db, O_RDONLY, 0644) )
	return 0;

      dbiIndexSet matches;

      rc = findPackageByLabel(db, name, &matches);
      if(rc==1)
	KpMsgE(i18n("Package %s is not installed\n"),name,TRUE);
      else if(rc==2)
	KpMsgE(i18n("Error looking for package %s\n"),name,TRUE);
      else
	{
	  int i;
	  for(i=0; i<matches.count; i++)
	    {
	      if(matches.recs[i].recOffset)
		{
		  h = rpmdbGetRecord(db, matches.recs[i].recOffset);
		  if(!h)
		    KpMsgE(i18n("Could not read database record\n"),"",TRUE);
		  else
		    {
		      filelist = RPM::collectFileList(h);
		      headerFree(h);
		    }
		}
	    }
	  dbiFreeIndexRecord(matches);
	}
      rpmdbClose(db);
      break;

      ////////////////////////////////////////////////////////////////////
      // query an uninstalled package      
    case 'u':
      name = fn.data();
      if((fd = fdOpen(name, O_RDONLY, 0)) < 0)
	{
	  KpMsgE(i18n("ERROR opening %s\n"),name,TRUE);
	  return filelist;
	}
#ifdef  RPMTAG_EPOCH
      if(fd!=0)
#else
      if(fd>=0)
#endif	
	{
	  rc = rpmReadPackageHeader(fd, &h, &isSource, NULL, NULL);
	  fdClose(fd);
	}
      switch(rc)
	{
	case 0:
	  if(!h)
	    KpMsgE(i18n("Old format source packages cannot be queried\n"),"",TRUE);
	  else
	    {
	      filelist = RPM::collectFileList(h);
	      headerFree(h);
	    }
	  break;
	case 1:
	  KpMsgE(i18n("%s does not appear to be a RPM package\n"),name,TRUE);
	  // fallthrough
	case 2:
	  KpMsgE(i18n("Query of %s failed\n"),name,TRUE);
	  return filelist;
	}
      break;
    }
   
  return filelist;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QList<char>* RPM::collectFileList(Header h)
{
  int_32 type, count;
  char **fileList;
  QList<char>* files = new QList<char>;

  kpkg->kp->setStatus(i18n("Getting file list"));
  kpkg->kp->setPercent(0);

  if(!headerGetEntry(h, RPMTAG_FILENAMES, &type, (void **) &fileList, &count))
    {
      kpkg->kp->setPercent(100);
      return files;
    }
  else
    {
      int i;
      for(i=0; i<count; i++)
	{
	  kpkg->kp->setPercent( (i/count) * 100);
	  files->append(fileList[i]);
	}
      kpkg->kp->setPercent(100);
      return files;
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QList<char> *RPM::depends(const char *name, int src)
{
  kpkg->kp->setStatus(i18n("Depends"));
  kpkg->kp->setPercent(0);

  int verifyFlags = 0;
  verifyFlags |= VERIFY_DEPS;

  const char *fls[2];
  fls[0] = name;
  fls[1] = NULL;

  QList<char> *result = new QList<char>;
  result->setAutoDelete(TRUE);

  doVerify("/", (enum verifysources) src, fls, verifyFlags, result);
  return result;  
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QList<char> *RPM::verify(packageInfo *p, QList<char> *files)
{
  kpkg->kp->setStatus(i18n("Verifying"));
  kpkg->kp->setPercent(0);

  int verifyFlags = 0;
  verifyFlags |= VERIFY_FILES |  VERIFY_SCRIPT |  VERIFY_MD5;

  const char *fls[2];
  fls[0] = p->getProperty("name")->data();
  fls[1] = NULL;

  QList<char> *result = new QList<char>;
  result->setAutoDelete(TRUE);

  doVerify("/", VERIFY_PACKAGE, fls, verifyFlags, result);
  return result;  
  
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int RPM::uninstall(int uninstallFlags, QList<packageInfo> *plist)
{
  int ln = plist->count() + 1;
  char **f = new char *[ln];

  packageInfo *pk;
  int i = 0;
  for (pk = plist->first(); pk != 0; pk = plist->next()) {
    f[i] = pk->getProperty("name")->data();
    i++;
  }
  f[i] = 0;

  int n =  doUninst(uninstallFlags,f);
  delete [] f;
  return n;
}

int RPM::uninstall(int uninstallFlags, packageInfo *p)
{
  char *files[2];
  files[0]=p->getProperty("name")->data();
  files[1]=NULL;

  return doUninst(uninstallFlags,files);
}

int RPM::doUninst(int uninstallFlags, char *files[])
{
  int interfaceFlags = 0 , uninstFlags = 0, test = 0;

  if ((uninstallFlags>>0 & 1) ^ puninstall[0].invert)
#ifdef  RPMTAG_EPOCH
    uninstFlags|=RPMTRANS_FLAG_NOSCRIPTS;
#else
    uninstFlags|=RPMUNINSTALL_NOSCRIPTS;
#endif  
  if ((uninstallFlags>>1 & 1) ^ puninstall[1].invert)
    interfaceFlags|=UNINSTALL_NODEPS;
  if ((uninstallFlags>>2 & 1) ^ puninstall[2].invert) {
#ifdef  RPMTAG_EPOCH
    uninstFlags|= RPMTRANS_FLAG_TEST;
#else
    uninstFlags|=RPMUNINSTALL_TEST;
#endif
    test = 1;
  }

  return (doUninstal("/", (const char **)files, uninstFlags, interfaceFlags) || test);
  
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int RPM::install(int installFlags, QList<packageInfo> *plist)
{
  int ln = plist->count() + 1;
  char **f = new char *[ln];

  packageInfo *pk;
  int i = 0;
  for (pk = plist->first(); pk != 0; pk = plist->next()) {
    QString fname = pk->fetchFilename();
    if (fname != "") {
      f[i] = strdup(fname.data());
      i++;
    }
  }
  f[i] = 0;

  int n =  doinst(installFlags,f);
  delete [] f;
  return n;
}

int RPM::install(int installFlags, packageInfo *p)
{
  char *files[2];
  QString fname = p->fetchFilename();
  files[0] = strdup(fname.data());
  files[1]=NULL;

  return doinst(installFlags,files);
}

int RPM::doinst(int installFlags, char *files[])
{
  int interfaceFlags = 0 , instFlags = 0, probFlags = 0, test=0;

  if ((installFlags>>0 & 1) ^ pinstall[0].invert)
    instFlags |= INSTALL_UPGRADE;
  if ((installFlags>>1 & 1) ^ pinstall[1].invert) {
#ifdef  RPMTAG_EPOCH
    probFlags|= RPMPROB_FILTER_REPLACEOLDFILES;
    probFlags|= RPMPROB_FILTER_REPLACENEWFILES;
#else
    instFlags|=RPMINSTALL_REPLACEFILES;
#endif    
  }
  if ((installFlags>>2 & 1) ^ pinstall[2].invert) {
#ifdef  RPMTAG_EPOCH
    probFlags|=RPMPROB_FILTER_REPLACEPKG;
    probFlags|=RPMPROB_FILTER_OLDPACKAGE;
#else
    instFlags|=RPMINSTALL_REPLACEPKG;
#endif    
      }
  if ((installFlags>>3 & 1) ^ pinstall[3].invert)
    interfaceFlags|=INSTALL_NODEPS;
  if ((installFlags>>4 & 1) ^ pinstall[4].invert) {
#ifdef  RPMTAG_EPOCH
    instFlags|=RPMTRANS_FLAG_TEST;
#else
    instFlags|=RPMINSTALL_TEST;
#endif    
    test = 1;
  }
  interfaceFlags|=INSTALL_PERCENT;

#ifdef  RPMTAG_EPOCH
  return (doInstal("/", (const char **)files, instFlags,
		interfaceFlags,  probFlags, 0) || test);
#else
  return (doInstal("/", files, instFlags,
  		interfaceFlags) || test);
#endif    
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QString RPM::FindFile(const char *name) {
  rpmdb db;
  QString s =  "";
  int i;
  Header h;
  dbiIndexSet matches;
  char *n;
  int_32 count, type;
 
  if (! rpmdbOpen("/", &db, O_RDONLY, 0644)) {
    if (rpmdbFindByFile(db, (char *)name, &matches) == 0) {
      for (i = 0; i < matches.count; i++) {
	if (matches.recs[i].recOffset) {
	  h = rpmdbGetRecord(db, matches.recs[i].recOffset);
	  if (h) {
	    headerGetEntry(h, RPMTAG_NAME, &type, (void **) &n, &count);
	    s += n;
	    s += "\t";
	    s += name;
	    s += "\n";
	    headerFree(h);
	  }
	}
      }
      dbiFreeIndexRecord(matches);
    }    
    rpmdbClose(db);
  }
  return s;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void RPM::setLocation()
{
    locatedialog->restore();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void RPM::setAvail(LcacheObj *slist)
{
  if (packageLoc)
    delete packageLoc;
  packageLoc = slist;
}
#else
  #include "rpmInterface.h"

  RPM::RPM(){}
  RPM::~RPM(){}
  
  bool RPM::isType(char *buf, const char *fname){return 0;}
  param *RPM::initinstallOptions(){return 0;}
  param *RPM::inituninstallOptions(){return 0;}
  packageInfo *RPM::getPackageInfo(char mode, const char *name, const char *version){return 0;}
  QList<char> *RPM::getFileList(packageInfo *p){return 0;}
  QList<char> *RPM::depends(const char *name, int src){return 0;}
  QList<char> *RPM::verify(packageInfo *p, QList<char> *files){return 0;}

  int RPM::uninstall(int uninstallFlags, QList<packageInfo> *p){return 0;}
  int RPM::uninstall(int uninstallFlags, packageInfo *p){return 0;}
  int RPM::doUninst(int uninstallFlags, char *files[]){return 0;}

  int RPM::install(int installFlags, QList<packageInfo> *p){return 0;}
  int RPM::install(int installFlags, packageInfo *p){return 0;}
  int RPM::doinst(int installFlags, char *files[]){return 0;}

  QString RPM::FindFile(const char *name){return 0;;}
  void RPM::collectDepends(packageInfo *p, const char *name, int src){}
  bool RPM::parseName(QString name, QString *n, QString *v){return 0;}

  void RPM::setLocation(){}
  void RPM::setAvail(LcacheObj *){}

  packageInfo* RPM::collectInfo(Header h){return 0;}
  QList<char>* RPM::collectFileList(Header h){return 0;}
  void RPM::listInstalledPackages(QList<packageInfo> *pki){}

#endif

